/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2003 by The BRLTTY Team. All rights reserved.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation.  Please see the file COPYING for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* BrailleLite/braille.c - Braille display library
 * For Blazie Engineering's Braille Lite series
 * Author: Nikhil Nair <nn201@cus.cam.ac.uk>
 * Copyright (C) 1998 by Nikhil Nair.
 * Some additions by: Nicolas Pitre <nico@cam.org>
 * Some modifications copyright 2001 by St�phane Doyon <s.doyon@videotron.ca>.
 * Some additions by: Dave Mielke <dave@mielke.cc>
 */

#define VERSION \
"Braille Lite driver, version 0.6.0 (June 2003)"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/termios.h>
#include <string.h>

#define BL_NEED_ARRAYS
#include "Programs/brl.h"
#include "Programs/misc.h"
#include "Programs/message.h"

#include "brlconf.h"

typedef enum {
  PARM_BAUDRATE=0,
  PARM_KBEMU
} DriverParameter;
#define BRLPARMS "baudrate", "kbemu"

#include "Programs/brl_driver.h"

#include "bindings.h"		/* for keybindings */


#define QSZ 256			/* size of internal input queue in bytes */
#define INT_CSR_SPEED 2		/* on/off time in cycles */
#define ACK_TIMEOUT 1000	/* timeout in ms for an ACK to come back */

int blite_fd = -1;		/* file descriptor for Braille display */

static unsigned char blitetrans[256];	/* dot mapping table (output) */
static unsigned char revtrans[256];	/* mapping for reversed display */
static unsigned char *prevdata;	/* previously received data */
static unsigned char *rawdata;	/* writebrl() buffer for raw Braille data */
static struct termios oldtio;	/* old terminal settings */
static int blitesz;	/* set to 18 or 40 */
static int waiting_ack = 0;	/* waiting acknowledgement flag */
static int reverse_kbd = 0;	/* reverse keyboard flag */
static int intoverride = 0;	/* internal override flag -
				 * highly dubious behaviour ...
				 */
static int int_cursor = 0;	/* position of internal cursor: 0 = none */
static int kbemu = 0; /* keyboard emulation (whether you can type) */

/* The input queue is only manipulated by the qput() and qget()
 * functions.
 */
static unsigned char *qbase;	/* start of queue in memory */
static int qoff = 0;		/* offset of first byte */
static int qlen = 0;		/* number of items in the queue */


/* Data type for a Braille Lite key, including translation into command
 * codes:
 */
typedef struct
  {
    unsigned char raw;		/* raw value, after any keyboard reversal */
    int cmd;			/* command code */
    unsigned char asc;		/* ASCII translation of Braille keys */
    unsigned char spcbar;	/* 1 = on, 0 = off */
    unsigned char routing;	/* routing key number */
  }
blkey;


/* Local function prototypes: */
static void getbrlkeys (void);	/* process keystrokes from the Braille Lite */
static int qput (unsigned char);	/* add a byte to the input queue */
static int qget (blkey *);	/* get a byte from the input queue */


static void
brl_identify (void)
{
  LogPrint(LOG_NOTICE, VERSION);
  /* I don't mean to take away anyone's copyright, but now that significant
     modifs have been made in more recent years by 2-3 others... it'd become
     bulky IMHO so let's just live without the copyright notice. */
  /*LogPrint(LOG_INFO, "   Copyright (C) 1998 by Nikhil Nair.");*/
}

static void
init_maps (void) {
  static unsigned char standard[] = {0, 1, 2, 3, 4, 5, 6, 7};	/* BRLTTY standard mapping */
  static unsigned char Blazie[]   = {0, 3, 1, 4, 2, 5, 6, 7};	/* Blazie standard */
  int byte;			/* loop counters */

  memset(blitetrans, 0, 256);	/* ordinary dot mapping */
  memset(revtrans, 0, 256);	/* display reversal - extra mapping */

  for (byte = 0; byte < 256; byte++) {
    int bit;
    for (bit = 0; bit < 8; bit++)
      if (byte & (1 << standard[bit]))
        blitetrans[byte] |= 1 << Blazie[bit];
    for (bit = 0; bit < 8; bit++) {
      revtrans[byte] <<= 1;
      if (byte & (1 << bit))
        revtrans[byte] |= 1;
    }
  }
}

static int
send_prebrl (int forever) {
  static unsigned char request[] = {0X05, 0X44};			/* code to send before Braille */
  if (forever) {
    /* wait forever method */
    while (1) {
      write(blite_fd, request, sizeof(request));
      waiting_ack = 1;
      delay(100);
      getbrlkeys();
      if (!waiting_ack) break;
      delay(2000);
    }
  } else {
    int timeout = ACK_TIMEOUT / 10;
    write(blite_fd, request, sizeof(request));
    waiting_ack = 1;
    while (1) {
      delay(10);	/* sleep for 10 ms */
      getbrlkeys();
      if (!waiting_ack) break;
      if (--timeout < 0) return 0;
    }
  }
  return 1;
}

static int
brl_open (BrailleDisplay *brl, char **parameters, const char *brldev)
{
  static unsigned good_baudrates[] =
    {300,600,1200,2400,4800,9600,19200,38400, 0};
  speed_t baudrate;
  /* Init string for Model detection */

  if (!*parameters[PARM_BAUDRATE] ||
      !validateBaud(&baudrate, "baud rate",
		    parameters[PARM_BAUDRATE], good_baudrates))
    baudrate = BAUDRATE;

  if (*parameters[PARM_KBEMU])
    validateYesNo(&kbemu, "keyboard emulation initial state",
		  parameters[PARM_KBEMU]);
  kbemu = !!kbemu;

  if ((qbase = (unsigned char *) malloc(QSZ))) {
    /* Open the Braille display device for random access */
    LogPrint(LOG_DEBUG, "Opening serial port: %s", brldev);
    if ((blite_fd = open(brldev, O_RDWR | O_NOCTTY)) != -1) {
      tcflush(blite_fd, TCIFLUSH);	/* clean line */

      /* save current settings */
      if (tcgetattr(blite_fd, &oldtio) != -1) {
        struct termios newtio;	/* new terminal settings */
        /* Set bps, flow control and 8n1, enable reading */
        newtio.c_cflag = baudrate | CRTSCTS | CS8 | CLOCAL | CREAD;
        LogPrint(LOG_DEBUG, "Selecting baudrate %d",
                 baud2integer(baudrate));

        /* Ignore bytes with parity errors and make terminal raw and dumb */
        newtio.c_iflag = IGNPAR;
        newtio.c_oflag = 0;		/* raw output */
        newtio.c_lflag = 0;		/* don't echo or generate signals */
        newtio.c_cc[VMIN] = 0;	/* set nonblocking read */
        newtio.c_cc[VTIME] = 0;

        /* activate new settings */
        if (tcsetattr(blite_fd, TCSANOW, &newtio) != -1) {
#ifdef DETECT_FOREVER
          if (send_prebrl(1)) {
#else /* DETECT_FOREVER */
          if (send_prebrl(0)) {
#endif /* DETECT_FOREVER */
            LogPrint(LOG_DEBUG, "Got response.");

            /* Next, let's detect the BLT-Model (18 || 40). */
            {
              unsigned char BltLen = 18;
              unsigned char cells[BltLen];
              memset(cells, 0, BltLen);
              write(blite_fd, cells, BltLen);
              waiting_ack = 1;
              delay(400);
              getbrlkeys();
              if (waiting_ack) {
                /* no response, so it must be BLT40 */
                BltLen = 40;
                brl->helpPage = 1;
              } else {
                brl->helpPage = 0;
              }
              LogPrint(LOG_NOTICE, "Braille Lite %d detected.", BltLen);

              brl->x = blitesz = BltLen;	/* initialise size of display - */
              brl->y = 1;			/* Braille Lites are single line displays */
            }

            {
              static unsigned char request[] = {0X05, 0X57};			/* code to send before Braille */
              delay(200);
              write(blite_fd, request, sizeof(request));
              waiting_ack = 0;
              delay(200);
              getbrlkeys();
              if (qlen) {
                unsigned char response[qlen + 1];
                int length = 0;
                do {
                  unsigned char byte = qbase[qoff % QSZ];
                  qoff = (qoff + 1) % QSZ, --qlen;
                  if (!byte) break;
                  response[length++] = byte;
                } while (qlen);
                response[length] = 0;
                LogPrint(LOG_NOTICE, "Braille Lite identity: %s", response);
              }
            }

            /* Allocate space for buffers */
            if ((prevdata = (unsigned char *) malloc(brl->x))) {
              memset(prevdata, 0, brl->x);
              if ((rawdata = (unsigned char *) malloc(brl->x))) {
                init_maps();
                return 1;
                free(rawdata);
              } else {
                LogPrint(LOG_ERR, "Cannot allocate rawdata.");
              }
              free(prevdata);
            } else {
              LogPrint(LOG_ERR, "Cannot allocate prevdata.");
            }
          } else {
            LogPrint(LOG_WARNING, "BLT doesn't seem to be connected, giving up!");
          }
          tcsetattr(blite_fd, TCSANOW, &oldtio);
        } else {
          LogPrint(LOG_ERR, "tcsetattr: %s", strerror(errno));
        }
      } else {
        LogPrint(LOG_ERR, "tcgetattr: %s", strerror(errno));
      }
      close(blite_fd);
    } else {
      LogPrint(LOG_ERR, "Open %s: %s", brldev, strerror(errno));
    }
    free(qbase);
  } else {
    LogPrint(LOG_ERR, "Cannot allocate qbase.");
  }
  return 0;
}


static void
brl_close (BrailleDisplay * brl)
{
#if 0 /* SDo: Let's leave the BRLTTY exit message there. */
  /* We just clear the display, using writebrl(): */
  memset (brl->buffer, 0, brl->x);
  brl_writeWindow (brl);
#endif /* 0 */

  free (prevdata);
  free (rawdata);
  free (qbase);

  if(blite_fd >= 0) {
    tcsetattr (blite_fd, TCSADRAIN, &oldtio);	/* restore terminal settings */
    close (blite_fd);
  }
}


static void
brl_writeStatus (BrailleDisplay *brl, const unsigned char *s)
{
}


static void
brl_writeWindow (BrailleDisplay * brl)
{
  short i;			/* loop counter */
  static int timer = 0;		/* for internal cursor */
  int timeout;			/* while waiting for an ACK */

  /* If the intoverride flag is set, then calls to writebrl() from the main
   * module are ignored, because the display is in internal use.
   * This is highly antisocial behaviour!
   */
  if (intoverride)
    return;

  /* First, the internal cursor: */
  if (int_cursor)
    {
      timer = (timer + 1) % (INT_CSR_SPEED * 2);
      if (timer < INT_CSR_SPEED)
	brl->buffer[int_cursor - 1] = 0x55;
      else
	brl->buffer[int_cursor - 1] = 0xaa;
    }

  /* Next we must handle display reversal: */
  if (reverse_kbd)
    for (i = 0; i < blitesz; i++)
      rawdata[i] = revtrans[brl->buffer[blitesz - 1 - i]];
  else
    memcpy (rawdata, brl->buffer, blitesz);

  /* Only refresh display if the data has changed: */
  if (memcmp (rawdata, prevdata, blitesz))
    {
      /* Save new Braille data: */
      memcpy (prevdata, rawdata, blitesz);

      /* Dot mapping from standard to BrailleLite: */
      for (i = 0; i < blitesz; i++)
	rawdata[i] = blitetrans[rawdata[i]];

      /* First we process any pending keystrokes, just in case any of them
       * are ^e ...
       */
      waiting_ack = 0;		/* Not really necessary, but ... */
      getbrlkeys ();

      /* Next we send the ^eD sequence, and wait for an ACK */
      waiting_ack = 1;
      /* send the ^ED... */
      if (!send_prebrl(0)) return;

      /* OK, now we'll suppose we're all clear to send Braille data. */
      write (blite_fd, rawdata, blitesz);

      /* And once again we wait for acknowledgement. */
      waiting_ack = 1;
      timeout = ACK_TIMEOUT / 10;
      do {
	  getbrlkeys ();
	  delay (10);		/* sleep for 10 ms */
	  if (--timeout < 0)
	    return; /* BLT doesn't seem to be connected any more! */
	}
      while (waiting_ack);
    }
}


static int
brl_readCommand (BrailleDisplay *brl, DriverCommandContext cmds)
{
  static int state = 0;		/* 0 = normal - transparent
				 * 1 = positioning internal cursor
				 * 2 = setting repeat count
				 * 3 = configuration options
				 */
  static int repeat = 0;		/* repeat count for command */
  static int repeatNext = 0; /* flag to indicate  whether 0 we repeat the
				same command or 1 we get the next command
				and repeat that. */
  static int hold, shift, shiftlck, ctrl, meta;
#ifdef USE_TEXTTRANS
  static int dot8shift;
#endif /* USE_TEXTTRANS */
  static blkey key;
  static unsigned char outmsg[41];
  int temp = CMD_NOOP;

 again:
  if(repeatNext || repeat == 0) {
    /* Process any new keystrokes: */
    getbrlkeys ();
    if (qget (&key) == EOF)	/* no keys to process */
      return EOF;
    repeatNext = 0;
  }
  if(repeat>0)
    repeat--;

  /* Our overall behaviour depends on the state variable (see above). */
  switch (state)
    {
    case 0:			/* transparent */
      /* First we deal with external commands: */
      do
	{
	  /* if it's not an external command, go on */
	  if (!key.cmd)
	    break;

	  /* if advance bar, return with corresponding command */
	  if (key.asc == 0)
	    return key.cmd;

	  /* I thought I was smart when I suggested to remove CMD_CUT_END.
	     Well now there's this nasty exception: the command offset
	     depends on the display size! */
	  if(key.cmd == CR_CUTRECT || key.cmd == CR_CUTLINE)
	    key.cmd += blitesz-1;

	  if(key.spcbar && (key.cmd &VAL_BLK_MASK) == VAL_PASSKEY) {
	    if(!kbemu)
	      return EOF;
	    if (!shiftlck)
	      shift = 0;
	    ctrl = meta = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif /* USE_TEXTTRANS */
	  }

	  /* always OK if chorded */
	  if (key.spcbar)
	    return key.cmd;

	  /* kbemu could be on, then go on */
	  if (kbemu && cmds == CMDS_SCREEN)
	    break;

	  /* if it's a dangerous command it should have been chorded */
	  if (dangcmd[(key.raw & 0x38) >> 3] & (1 << (key.raw & 0x07)))
	    break;

	  /* finally we are OK */
	  return key.cmd;
	}
      while (0);

      /* Next, internal commands: */
      if (key.spcbar)
	switch (key.asc)
	  {
	  case BLT_KBEMU:	/* set keyboard emulation */
	    kbemu ^= 1;
	    shift = shiftlck = ctrl = meta = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif /* USE_TEXTTRANS */
	    if(kbemu)
	      message ("keyboard emu on", MSG_SILENT);
	    else message ("keyboard emu off", MSG_SILENT);
	    return CMD_NOOP;
	  case BLT_ROTATE:	/* rotate Braille Lite by 180 degrees */
	    reverse_kbd ^= 1;
	    return CMD_NOOP;
	  case BLT_POSITN:	/* position internal cursor */
	    int_cursor = blitesz / 2;
	    state = 1;
	    return CMD_NOOP;
	  case BLT_REPEAT:	/* set repeat count */
	    hold = 0;
	    sprintf (outmsg, "Repeat count:");
	    message (outmsg, MSG_SILENT | MSG_NODELAY);
	    intoverride = 1;
	    state = 2;
	    return CMD_NOOP;
	  case BLT_CONFIG:	/* configuration menu */
	    sprintf (outmsg, "Config? [m/s/r/z]");
	    message (outmsg, MSG_SILENT | MSG_NODELAY);
	    intoverride = 1;
	    state = 3;
	    return CMD_NOOP;
	  case ' ':		/* practical exception for */
	    /* If keyboard mode off, space bar == CMD_HOME */
	    if (!kbemu || cmds != CMDS_SCREEN)
	      return CMD_HOME;
	  }

      /* check for routing keys */
      if (key.routing)
	return (CR_ROUTE + key.routing - 1);

      if (!kbemu)
	return CMD_NOOP;

      /* Now kbemu is definitely on. */
      switch (key.raw & 0xC0)
	{
	case 0x40:		/* dot 7 */
	  shift = 1;
	  break;
	case 0xC0:		/* dot 78 */
	  ctrl = 1;
	  break;
	case 0x80:		/* dot 8 */
#ifdef USE_TEXTTRANS
	  dot8shift = 1;
#else /* USE_TEXTTRANS */
	  meta = 1;
#endif /* USE_TEXTTRANS */
	  break;
	}

      if (key.spcbar && key.asc != ' ')
	switch (key.asc)
	  {
	  case BLT_UPCASE:	/* upper case next */
	    if (shift)
	      shiftlck = 1;
	    else
	      shift = 1;
	    return CMD_NOOP;
	  case BLT_UPCOFF:	/* cancel upper case */
	    shift = shiftlck = 0;
	    return CMD_NOOP;
	  case BLT_CTRL:	/* control next */
	    ctrl = 1;
	    return CMD_NOOP;
#ifdef USE_TEXTTRANS
	  case BLT_DOT8SHIFT:	/* add dot 8 to next pattern */
	    dot8shift = 1;
	    return CMD_NOOP;
#endif /* USE_TEXTTRANS */
	  case BLT_META:	/* meta next */
	    meta = 1;
	    return CMD_NOOP;
	  case BLT_ABORT:	/* abort - quit keyboard emulation */
	    kbemu = 0;
	    message ("keyboard emu off", MSG_SILENT);
	    return CMD_NOOP;
	  default:		/* unrecognised command */
	    shift = shiftlck = ctrl = meta = 0;
#ifdef USE_TEXTTRANS
	    dot8shift = 0;
#endif /* USE_TEXTTRANS */
	    return CMD_NOOP;
	  }

      /* OK, it's an ordinary (non-chorded) keystroke, and kbemu is on. */
#ifndef USE_TEXTTRANS
      if (ctrl && key.asc >= 96)
	/* old code was (key.asc & 0x1f) */
	temp = VAL_PASSCHAR | key.asc | VPC_CONTROL;
      else if (meta && key.asc >= 96)
	temp = VAL_PASSCHAR | key.asc | VPC_META;
      else if (shift && (key.asc & 0x40))
	/* old code was (key.asc & 0xdf) */
	temp = VAL_PASSCHAR | key.asc | VPC_SHIFT;
      else
	temp = VAL_PASSCHAR | key.asc;
#else /* USE_TEXTTRANS */
      temp = VAL_PASSDOTS |
	(keys_to_dots[key.raw &0x3F]
	 | ((meta) ? VPC_META : 0)
	 | ((ctrl) ? 0xC0 : 
	    (shift) ? 0x40 : 
	    (dot8shift) ? 0x80 : 0));
#endif /* USE_TEXTTRANS */
      if (!shiftlck)
	shift = 0;
      ctrl = meta = 0;
#ifdef USE_TEXTTRANS
      dot8shift = 0;
#endif /* USE_TEXTTRANS */
      outmsg[0] = 0;
      return temp;

    case 1:			/* position internal cursor */
      switch (key.cmd)
	{
	case CMD_HOME:		/* go to middle */
	  int_cursor = blitesz / 2;
	  break;
	case CMD_LNBEG:	/* beginning of display */
	  int_cursor = 1;
	  break;
	case CMD_LNEND:	/* end of display */
	  int_cursor = blitesz;
	  break;
	case CMD_FWINLT:	/* quarter left */
	  int_cursor = MAX (int_cursor - blitesz / 4, 1);
	  break;
	case CMD_FWINRT:	/* quarter right */
	  int_cursor = MIN (int_cursor + blitesz / 4, blitesz);
	  break;
	case CMD_CHRLT:	/* one character left */
	  if (int_cursor > 1)
	    int_cursor--;
	  break;
	case CMD_CHRRT:	/* one character right */
	  if (int_cursor < blitesz)
	    int_cursor++;
	  break;
	case CR_ROUTE:	/* route cursor */
	  if (key.spcbar)
	    {
	      temp = CR_ROUTE + int_cursor - 1;
	      int_cursor = state = 0;
	    }
	  return temp;
	case CR_CUTBEGIN:	/* begin cut */
	case CR_CUTAPPEND:
	  if (key.spcbar)
	    {
	      temp = key.cmd + int_cursor - 1;
	      int_cursor = state = 0;
	    }
	  return temp;
	case CR_CUTRECT:	/* end cut */
	case CR_CUTLINE:
	  if (key.spcbar)
	    {
	      temp = key.cmd + int_cursor - 1;
	      int_cursor = state = 0;
	    }
	  return temp;
	case CMD_DISPMD: /* attribute info */
	  temp = CR_DESCCHAR + int_cursor - 1;
	  int_cursor = state = 0;
	  return temp;
	default:
	  if (key.asc == BLT_ABORT)	/* cancel cursor positioning */
	    int_cursor = state = 0;
	  break;
	}
      if (key.routing)
	int_cursor = key.routing;
      return CMD_NOOP;
    case 2:			/* set repeat count */
      if (key.asc >= '0' && key.asc <= '9')
	{
	  hold = (hold * 10 + key.asc - '0') % 100;
	  if(hold)
	    sprintf (outmsg, "Repeat count: %d", hold);
	  else sprintf (outmsg, "Repeat count: ");
	  intoverride = 0;
	  message (outmsg, MSG_SILENT | MSG_NODELAY);
	  intoverride = 1;
	}
      else if (key.routing)
	{
	  hold = key.routing +1;
	  sprintf (outmsg, "Repeat count: %d", hold);
	  intoverride = 0;
	  message (outmsg, MSG_SILENT | MSG_NODELAY);
	  intoverride = 1;
	}
      else{
	intoverride = 0;
	outmsg[0] = 0;
	state = 0;
	if(hold > 0) {
	  if(key.asc == SWITCHVT_NEXT || key.asc == SWITCHVT_PREV)
	    /* That's chorded or not... */
	    return CR_SWITCHVT + (hold-1);
	  else if(key.asc == O_SETMARK)
	    return CR_SETMARK + (hold-1);
	  else if(key.asc == O_GOTOMARK)
	    return CR_GOTOMARK + (hold-1);
	  else if (key.spcbar)		/* chorded */
	    switch(key.asc)
	      {
	      case BLT_ENDCMD:	/* set repeat count */
		if (hold > 1) {
		  /* repeat next command */
		  repeat = hold;
		  repeatNext = 1;
		}
		/* fall through */
	      case BLT_ABORT:	/* abort or endcmd */
		return CMD_NOOP;
	      }
	  /* if the key is any other, start repeating it. */
	  repeat = hold;
	  goto again;
	}
      }
      return CMD_NOOP;
    case 3:			/* preferences options */
      switch (key.asc)
	{
	case 'm':		/* preferences menu */
	  intoverride = state = 0;
	  return CMD_PREFMENU;
	case 's':		/* save preferences */
	  intoverride = state = 0;
	  return CMD_PREFSAVE;
	case 'r':		/* restore saved preferences */
	  intoverride = state = 0;
	  return CMD_PREFLOAD;
	case BLT_ABORT:	/* abort */
	  intoverride = state = 0;
	default:		/* in any case */
	  return CMD_NOOP;
	}
    }

  /* We should never reach this point ... */
  return EOF;
}


static void
getbrlkeys (void)
{
  unsigned char c;		/* character buffer */

  while (read (blite_fd, &c, 1))
    {
      if (waiting_ack && c == 5)	/* ^e is the acknowledgement character ... */
	waiting_ack = 0;
      else
	qput (c);
    }
}


static int
qput (unsigned char c)
{
  if (qlen == QSZ)
    return EOF;
  qbase[(qoff + qlen++) % QSZ] = c;
  return 0;
}


static int
qget (blkey * kp)
{
  unsigned char c;
  int how;
  static unsigned char counts[] = {1, 3, 3};
  unsigned char count;

  if (qlen == 0)
    return EOF;
  c = qbase[qoff];

  /* extended sequences start with a zero */
  how = (c == 0)? 1:
        (c == 0x80 && blitesz != 18)? 2:
        0;
  count = counts[how];
  if (qlen < count)
    return EOF;

  memset (kp, 0, sizeof (*kp));
  switch (how) {
    case 0: /* non-extended sequences (BL18) */
      /* We must deal with keyboard reversal here: */
      if (reverse_kbd)
	{
	  if (c >= 0x80)	/* advance bar */
	    c ^= 0x03;
	  else
	    c = (c & 0x40) | ((c & 0x38) >> 3) | ((c & 0x07) << 3);
	}

      /* Now we fill in all the info about the keypress: */
      if (c >= 0x80)		/* advance bar */
	{
	  kp->raw = c;
	  switch (c)
	    {
	    case 0x83:		/* left */
	      kp->cmd = BLT_BARLT;
	      break;
	    case 0x80:		/* right */
	      kp->cmd = BLT_BARRT;
	      break;
	    default:		/* unrecognised keypress */
	      kp->cmd = 0;
              break;
	    }
	}
      else
	{
	  kp->spcbar = ((c & 0x40)? 1: 0);
	  c &= 0x3f;		/* leave only dot key info */
	  kp->raw = c;
	  kp->cmd = cmdtrans[c];
	  kp->asc = brltrans[c];
	}
      break;

    case 1: { /* extended sequences (BL40) */
      unsigned char c2 = qbase[((qoff + 1) % QSZ)];
      unsigned char c3 = qbase[((qoff + 2) % QSZ)];

      /* We must deal with keyboard reversal here: */
      if (reverse_kbd)
	{
	  if (c2 == 0)
	    {			/* advance bars or routing keys */
	      if (c3 & 0x80)	/* advance bars */
		c3 = ((c3 & 0xF0) |
		      ((c3 & 0x1) << 3) | ((c3 & 0x2) << 1) |
		      ((c3 & 0x4) >> 1) | ((c3 & 0x8) >> 3));
	      else if (c3 > 0 && c3 <= blitesz)
		c3 = blitesz - c3 + 1;
	    }
	  else
            {
              c2 = (((c2 & 0x38) >> 3) | ((c2 & 0x07) << 3) |
                    ((c2 & 0x40) << 1) | ((c2 & 0x80) >> 1));
              c3 = ((c3 & 0x40) | ((c3 & 0x38) >> 3) | ((c3 & 0x07) << 3));
            }
	}

      /* Now we fill in all the info about the keypress: */
      if (c2 == 0)		/* advance bars or routing keys */
	{
	  kp->raw = c3;
	  if (c3 & 0x80)
            kp->cmd = barcmds[c3 & 0xF];
	  else if (c3 > 0 && c3 <= blitesz)
	    kp->routing = c3;
	}
      else
	{
	  kp->spcbar = ((c3 & 0x40)? 1: 0);
	  c3 &= 0x3f;		/* leave only dot key info */
	  kp->raw = (c2 & 0xC0) | c3; /* combine info for all 8 dots */
	  /* c2&0x3F and c3&0x3F are the same, i.e. dots 1-6. */
	  kp->cmd = cmdtrans[c3];
	  kp->asc = brltrans[c3];
	}
      break;
    }

    case 2: { /* extended sequences (millennium) */
      unsigned char c3 = qbase[((qoff + 2) % QSZ)];

      /* We must deal with keyboard reversal here: */
      if (reverse_kbd)
        c3 = ((c3 & 0x11) << 3) | ((c3 & 0x22) << 1) | ((c3 & 0x44) >> 1) | ((c3 & 0x88) >> 3);
      kp->raw = c3;

      if (c3 & 0xf)
        kp->cmd = barcmds[((c3 & 0x1) << 3) | ((c3 & 0x2) << 1) | ((c3 & 0x4) >> 1) | ((c3 & 0x8) >> 3)];
      else if (c3 & 0x10)
        kp->cmd = BLT_WHLUP2;
      else if (c3 & 0x20)
        kp->cmd = BLT_WHLDN2;
      else if (c3 & 0x40)
        kp->cmd = BLT_WHLUP1;
      else if (c3 & 0x80)
        kp->cmd = BLT_WHLDN1;
      else
        kp->cmd = 0;
      break;
    }

    default:
      kp->cmd = 0;
      break;
  }

  /* adjust queue variables for next member */
  qoff = (qoff + count) % QSZ;
  qlen -= count;

  return 0;
}

/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2004 by The BRLTTY Team. All rights reserved.
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

#ifndef _BRL_H
#define _BRL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <unistd.h>

#include "brldefs.h"

/* status cell styles */
typedef enum {
  ST_None,
  ST_AlvaStyle,
  ST_TiemanStyle,
  ST_PB80Style,
  ST_Generic,
  ST_MDVStyle,
  ST_VoyagerStyle
} StatusCellsStyle;

/* Braille information structure. */
typedef struct {
  int x, y;			/* the dimensions of the display */
  int helpPage;			/* the page number within the help file */
  unsigned char *buffer;	/* the contents of the display */
  unsigned isCoreBuffer:1;	/* the core allocated the buffer */
  unsigned resizeRequired:1;	/* the display size has changed */
  unsigned int writeDelay;
  void (*bufferResized) (int rows, int columns);
  const char *dataDirectory;
} BrailleDisplay;				/* used for writing to a braille display */

extern void initializeBrailleDisplay (BrailleDisplay *);
extern unsigned int drainBrailleOutput (BrailleDisplay *, int minimumDelay);
extern int allocateBrailleBuffer (BrailleDisplay *);

extern void writeBrailleBuffer (BrailleDisplay *);
extern void writeBrailleText (BrailleDisplay *, const char *, int);
extern void writeBrailleString (BrailleDisplay *, const char *);
extern void showBrailleString (BrailleDisplay *, const char *, unsigned int);

extern void clearStatusCells (BrailleDisplay *brl);
extern void setStatusText (BrailleDisplay *brl, const char *text);

extern int readBrailleCommand (BrailleDisplay *, BRL_DriverCommandContext);
#define IS_DELAYED_COMMAND(cmd) (((cmd) & BRL_FLG_REPEAT_DELAY) && !((cmd) & BRL_FLG_REPEAT_INITIAL))

typedef enum {
  BF_MINIMUM,
  BF_LOW,
  BF_MEDIUM,
  BF_HIGH,
  BF_MAXIMUM
} BrailleFirmness;

/* Routines provided by each braille driver.
 * These are loaded dynamically at run-time into this structure
 * with pointers to all the functions and variables.
 */
typedef struct {
  const char *name;			/* name of driver */
  const char *identifier;		/* name of driver */
  const char *date;		/* compilation date of driver */
  const char *time;		/* compilation time of driver */
  const char *const *parameters;	/* user-supplied driver parameters */
  const char *helpFile;		/* name of help file */
  int statusStyle;		/* prefered status cells mode */

  /* Routines provided by the braille driver library: */
  void (*identify) (void);	/* print start-up messages */
  int (*open) (BrailleDisplay *, char **parameters, const char *);	/* initialise Braille display */
  void (*close) (BrailleDisplay *);		/* close braille display */
  int (*readCommand) (BrailleDisplay *, BRL_DriverCommandContext);		/* get key press from braille display */
  void (*writeWindow) (BrailleDisplay *);		/* write to braille display */
  void (*writeStatus) (BrailleDisplay *brl, const unsigned char *);	/* set status cells */

  /* These require BRL_HAVE_VISUAL_DISPLAY. */
  void (*writeVisual) (BrailleDisplay *);		/* write to braille display */

  /* These require BRL_HAVE_PACKET_IO. */
  ssize_t (*readPacket) (BrailleDisplay *, unsigned char *, size_t);
  ssize_t (*writePacket) (BrailleDisplay *, const unsigned char *, size_t);
  int (*reset) (BrailleDisplay *);
  
  /* These require BRL_HAVE_KEY_CODES. */
  int (*readKey) (BrailleDisplay *);
  int (*keyToCommand) (BrailleDisplay *, BRL_DriverCommandContext, int);

  /* These require BRL_HAVE_FIRMNESS. */
  void (*firmness) (BrailleDisplay *brl, BrailleFirmness setting);		/* mute speech */
} BrailleDriver;

extern const BrailleDriver *loadBrailleDriver (const char *identifier, void **driverObject, const char *driverDirectory);
extern void identifyBrailleDrivers (void);
extern int listBrailleDrivers (const char *directory);
extern const BrailleDriver *braille;
extern const BrailleDriver noBraille;

typedef unsigned char TranslationTable[0X100];
extern TranslationTable textTable;	 /* current text to braille translation table */
extern TranslationTable untextTable;     /* current braille to text translation table */
extern TranslationTable attributesTable; /* current attributes to braille translation table */
extern void *contractionTable; /* current braille contraction table */

typedef unsigned char DotsTable[8];
extern void makeOutputTable (const DotsTable *dots, TranslationTable *table);

/* Formatting of status cells. */
extern const unsigned char landscapeDigits[11];
extern int landscapeNumber (int x);
extern int landscapeFlag (int number, int on);
extern const unsigned char seascapeDigits[11];
extern int seascapeNumber (int x);
extern int seascapeFlag (int number, int on);
extern const unsigned char portraitDigits[11];
extern int portraitNumber (int x);
extern int portraitFlag (int number, int on);

extern void learnMode (BrailleDisplay *brl, int poll, int timeout);

extern void showDotPattern (unsigned char dots, unsigned char duration);
extern void setBrailleFirmness (BrailleDisplay *brl, int setting);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BRL_H */

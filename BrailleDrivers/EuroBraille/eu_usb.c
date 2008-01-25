/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2008 by The BRLTTY Developers.
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

/* EuroBraille -- Input/Output handling
** USB Specific low-level IO routines.
*/

#include "prologue.h"

#include <errno.h>

#include "eu_io.h"

#ifdef ENABLE_USB_SUPPORT
#include "io_usb.h"

static UsbChannel *usb = NULL;
int
eubrl_usbInit (BrailleDisplay *brl, char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { /* ESYS 12/40 */
      .vendor=0XC251, .product=0X1122,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=1,
      .disableAutosuspend=1
    }
    ,
    { .vendor=0 }
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) 
    {
      usbBeginInput(usb->device, usb->definition.inputEndpoint, 8);
      return 1;
    }
  return 0;
}

int
eubrl_usbClose (BrailleDisplay *brl) 
{
  if (usb) {
    usbCloseChannel(usb);
    usb = NULL;
  }
  return 0;
}


ssize_t
eubrl_usbRead (BrailleDisplay *brl, void *buffer, size_t length) 
{
  ssize_t count = usbReapInput(usb->device, usb->definition.inputEndpoint, buffer, length, 0, 0);
  if (count == -1)
    if (errno == EAGAIN)
      count = 0;
  return count;
}

ssize_t
eubrl_usbWrite(BrailleDisplay *brl, const void *buffer, size_t length)
{
  return usbWriteEndpoint(usb->device, usb->definition.outputEndpoint, buffer, length, 1000);
}


#else /* No usb support - empty functions */


int
eubrl_usbInit (BrailleDisplay *brl, char **parameters, const char *device)
{
  return 0;
}

int
eubrl_usbClose (BrailleDisplay *brl) 
{
  return (0);
}


ssize_t
eubrl_usbRead (BrailleDisplay *brl, void *buffer, size_t length) 
{
  return -1;
}

ssize_t
eubrl_usbWrite(BrailleDisplay *brl, const void *buffer, size_t length)
{
  return -1;
}

#endif /* ENABLE_USB_SUPPORT */

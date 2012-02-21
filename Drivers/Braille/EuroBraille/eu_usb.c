/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2012 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

/* EuroBraille -- Input/Output handling
** USB Specific low-level IO routines.
*/

#include <string.h>

#include "prologue.h"

#include <errno.h>

#include "log.h"

#include "eu_io.h"

#define USB_PACKET_SIZE 64


#include "io_usb.h"

static UsbChannel *usb = NULL;
int
eubrl_usbInit (BrailleDisplay *brl, char **parameters, const char *device) {
  static const UsbChannelDefinition definitions[] = {
    { /* Esys, version < 3.0, without SD card */
      .vendor=0XC251, .product=0X1122,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1123,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esys, version < 3.0, with SD card */
      .vendor=0XC251, .product=0X1124,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1125,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esys, version >= 3.0, without SD card */
      .vendor=0XC251, .product=0X1126,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1127,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esys, version >= 3.0, with SD card */
      .vendor=0XC251, .product=0X1128,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1129,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* Esytime */
      .vendor=0XC251, .product=0X1130,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1131,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { /* reserved */
      .vendor=0XC251, .product=0X1132,
      .configuration=1, .interface=0, .alternative=0,
      .inputEndpoint=1, .outputEndpoint=0
    }
    ,
    { .vendor=0 }
  };

  if ((usb = usbFindChannel(definitions, (void *)device))) 
    {
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
eubrl_usbRead (BrailleDisplay *brl, void *buffer, size_t length, int wait) 
{
  int timeout = 20;
  ssize_t count = usbReapInput(usb->device, usb->definition.inputEndpoint, buffer, length, (wait? timeout: 0), timeout);

  return count;
}

ssize_t
eubrl_usbWrite(BrailleDisplay *brl, const void *buffer, size_t length)
{
  size_t pos = 0;
  while (pos < length) {
    char packetToSend[USB_PACKET_SIZE];
    size_t tosend = length - pos;
    if (tosend > USB_PACKET_SIZE) {
      tosend = USB_PACKET_SIZE;
    }
    memset(packetToSend,0x55,USB_PACKET_SIZE);
    memcpy(packetToSend,buffer+pos,tosend);
    if (usbHidSetReport(usb->device, usb->definition.interface, 0, packetToSend, USB_PACKET_SIZE, 10) < 0)
      return -1;
    pos += tosend;
  }
  return length;
}

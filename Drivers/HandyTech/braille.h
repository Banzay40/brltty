/*
 * BRLTTY - A background process providing access to the Linux console (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2005 by The BRLTTY Team. All rights reserved.
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

/* HandyTech/braille.h - Configurable definitions for the Handy driver
 * Copyright (C) 1995-1998 by Nicolas Pitre <nico@cam.org>
 *
 * Edit as necessary for your system.
 */

/***** User Settings *****/
/* typematic settings */
#define TYPEMATIC_DELAY 10	/* nbr of cycles before a key is repeated */
#define TYPEMATIC_REPEAT 2	/* nbr of cycles between each key repeat */


/* Delay in miliseconds between forced full refresh of the display.
 * This is to minimize garbage effects due to noise on the serial line.
 */
#define REFRESH_RATE 1000


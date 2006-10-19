###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2006 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation.  Please see the file COPYING for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

function writeCommandEntry(name, symbol, help) {
  print "{"
  print "  .name = \"" name "\","
  print "  .code = " symbol ","
  print "  .description = \"" help "\""
  print "},"
}

function brlCommand(name, symbol, value, help) {
  writeCommandEntry(name, symbol, help)
}

function brlBlock(name, symbol, value, help) {
  writeCommandEntry(name, symbol, help)
}

function brlKey(name, symbol, value, help) {
  writeCommandEntry(name, "BRL_BLK_PASSKEY+" symbol, help)
}

function brlFlag(name, symbol, value, help) {
}

function brlDot(number, symbol, value, help) {
}

"""
This module is a binding for BrlAPI, a BrlTTY bridge for applications.

C API documentation : http://mielke.cc/brltty/doc/BrlAPIref-HTML

Example : 
import brlapi
b = brlapi.Bridge()
b.getTty()
b.writeText("Press any key to continue ...")
key = b.readKey()
b.writeText("Key %lld ! Press any key to exit ..." % key)
b.readKey()
b.leaveTty()
"""

###############################################################################
# libbrlapi - A library providing access to braille terminals for applications.
#
# Copyright (C) 2005-2006 by Alexis Robert <alexissoft@free.fr>
#
# libbrlapi comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU Lesser General Public License, as published by the Free Software
# Foundation; either version 2.1 of the License,
# or (at your option) any later version.
# Please see the file COPYING-API for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

cimport c_brlapi

cdef returnerrno():
	"""This function returns str message error from errno"""
	cdef c_brlapi.brlapi_error_t *error
	error = c_brlapi.brlapi_error_location()
	return c_brlapi.brlapi_strerror(error)

class ConnectionError:
	"""Error while connecting to BrlTTY"""
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return self.value

class OperationError:
	"""Error while getting an attribute"""
	def __init__(self, value):
		self.value = value
	def __str__(self):
		return self.value

cdef class Write:
	"""Structure containing arguments to be given to Bridge.write()"""
	cdef c_brlapi.brlapi_writeStruct props

	def __new__(self):
		self.props.regionBegin = 0
		self.props.regionSize = 0
		self.props.text = ""
		# I must add attrAnd & attrOr
		self.props.cursor = 0
		self.props.charset = ""

	property regionBegin:
		"""Display number -1 == unspecified"""
		def __get__(self):
			return self.props.regionBegin
		def __set__(self, val):
			self.props.regionBegin = val

	property regionSize:
		"""Region of display to update, 1st character of display is 1"""
		def __get__(self):
			return self.props.regionSize
		def __set__(self, val):
			self.props.regionSize = val

	property text:
		"""Number of characters held in text, attrAnd and attrOr. For multibytes text, this is the number of multibyte characters. Combining and double-width characters count for 1"""
		def __get__(self):
			return self.props.text
		def __set__(self, val):
			self.props.text = val

	property cursor:
		"""Or attributes; applied after ANDing"""
		def __get__(self):
			return self.props.cursor
		def __set__(self, val):
			self.props.cursor = val

	property charset:
		"""-1 == don't touch, 0 == turn off, 1 = 1st char of display, ..."""
		def __get__(self):
			return self.props.charset
		def __set__(self, val):
			self.props.charset = val

	property attrAnd:
		"""Text to display"""
		def __get__(self):
			return <char*>self.props.attrAnd
		def __set__(self, val):
			self.props.attrAnd = <unsigned char*>val

	property attrOr:
		"""And attributes; applies first"""
		def __get__(self):
			return <char*>self.props.attrOr
		def __set__(self, val):
			self.props.attrOr = <unsigned char*>val

cdef class Settings:
	"""Settings structure for BrlAPI connection"""
	cdef c_brlapi.brlapi_settings_t props
	def __init__(self, authkey = None, hostname = None):
		if authkey:
			self.props.authKey = authkey
		else:
			self.props.authKey = ""

		if hostname:
			self.props.hostName = hostname
		else:
			self.props.hostName = ""

	property authkey:
		"""To get authorized to connect, libbrlapi has to tell the BrlAPI server a secret key, for security reasons. This is the path to the file which holds it; it will hence have to be readable by the application.
		
		Setting None defaults it to local installation setup or to the content of the BRLAPI_AUTHPATH environment variable, if it exists."""
		def __get__(self):
			return self.props.authKey
		def __set__(self, val):
			self.props.authKey = val

	property hostname:
		"""This tells where the BrlAPI server resides : it might be listening on another computer, on any TCP port. It should look like "foo:1", which means TCP port number BRLAPI_SOCKETPORTNUM+1 on computer called "foo".
		
		Note: Please check that resolving this name works before complaining.
		
		Settings None defaults it to localhost, using the local installation's default TCP port, or to the content of the BRLAPI_HOSTNAME environment variable, if it exists."""
		def __get__(self):
			return self.props.hostName
		def __set__(self, val):
			self.props.hostName = val

cdef class Bridge:
	"""Class which manages the bridge between BrlTTY and your program"""
	def __init__(self, Settings clientSettings = None, Settings usedSettings = None):
		self.connect(clientSettings, usedSettings)

	def __del__(self):
		self.close()

	def connect(self, Settings clientSettings, Settings usedSettings):
		"""Connect your program to BrlTTY using settings"""

		cdef c_brlapi.brlapi_settings_t *client
		cdef c_brlapi.brlapi_settings_t *used

		# Fix segmentation fault
		if clientSettings == None:
			client = NULL
		else:
			client = &clientSettings.props

		if usedSettings == None:
			used = NULL
		else:
			used = &usedSettings.props

		PY_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi_initializeConnection(client, used)
		PY_END_ALLOW_THREADS
		if retval == -1:
			if clientSettings == None:
				hostname = "localhost"
			else:
				hostname = clientSettings.hostname

			raise ConnectionError("couldn't connect to %s: %s" % (hostname,returnerrno()))

	def close(self):
		"""Close the BrlAPI conection"""
		c_brlapi.brlapi_closeConnection()

	# TODO: loadAuthKey

	property displaysize:
		"""Get the size of the braille display"""
		def __get__(self):
			cdef unsigned int x
			cdef unsigned int y
			PY_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi_getDisplaySize(&x, &y)
			PY_END_ALLOW_THREADS
			if retval == -1:
				raise OperationError(returnerrno())
			else:
				return (x, y)
	
	property driverid:
		"""Identify the driver used by BrlTTY"""
		def __get__(self):
			cdef char id[3]
			PY_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi_getDriverId(id, sizeof(id))
			PY_END_ALLOW_THREADS
			if retval == -1:
				raise OperationError(returnerrno())
			else:
				return id

	# TODO: getDriverInfo
	
	property drivername:
		"""Get the complete name of the driver used by BrlTTY"""
		def __get__(self):
			cdef char name[21]
			PY_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi_getDriverName(name, sizeof(name))
			PY_END_ALLOW_THREADS
			if retval == -1:
				raise OperationError(returnerrno())
			else:
				return name

	def getTty(self, tty = -1, how = None):
		"""Ask for some tty, with some key mechanism
		* tty : If tty >= 0, application takes control of the specified tty
			If tty == -1, the library first tries to get the tty number from the WINDOWID environment variable (form xterm case), then the CONTROLVT variable, and at last reads /proc/self/stat (on linux)
		* how : Tells how the application wants readKey() to return key presses. None or "" means BrlTTY commands are required, whereas a driver name means that raw key codes returned by this driver are expected."""
		if not how:
			PY_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi_getTty(tty, NULL)
			PY_END_ALLOW_THREADS
		else:
			PY_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi_getTty(tty, how)
			PY_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def leaveTty(self):
		"""Stop controlling the tty"""
		PY_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi_leaveTty()
		PY_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval
			

	# TODO : getTtyPath
	
	def setFocus(self, tty):
		"""Tell the current tty to brltty.
		This is intended for focus tellers, such as brltty, xbrlapi, screen, ... getTty() must have been called before hand to tell where this focus applies in the tty tree."""
		retval = c_brlapi.brlapi_setFocus(tty)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def write(self, Write writeStruct):
		"""Update a specific region of the braille display and apply and/or masks.
		* s : gives information necessary for the update"""
		retval = c_brlapi.brlapi_write(&writeStruct.props)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def writeDots(self, dots):
		"""Write the given dots array to the display.
		* dots : points on an array of dot information, one per character. Its size must hence be the same as what displaysize provides."""
		retval = c_brlapi.brlapi_writeDots(<unsigned char*>dots)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def writeText(self, str, cursor = 0):
		"""Write the given \0-terminated string to the braille display.
		If the string is too long, it is cut. If it's too short, spaces are appended. The current LC_CTYPE locale is considered, unless it is left as default "C", in which case the charset is assumed to be 8bits, and the same as the server's.

		* cursor : gives the cursor position; if equal to 0, no cursor is shown at all; if cursor == -1, the cursor is left where it is
		* str : points to the string to be displayed"""
		retval = c_brlapi.brlapi_writeText(cursor, str)
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	def readKey(self, block = True):
		"""Read a key from the braille keyboard.

		This function returns one key press's code.

		If None or "" was given to getTty(), a brltty command is returned. It is hence pretty driver-independent, and should be used by default when no other option is possible.

		By default, all commands but those which restart drivers and switch virtual are returned to the application and not to brltty. If the application doesn't want to see some command events, it should call either ignoreKeySet() or ignoreKeyRange().

		If some driver name was given to getTty(), a raw keycode is returned, as specified by the terminal driver. It generally corresponds to the very code that the terminal tells to the driver. This should only be used by applications which are dedicated to a particular braille terminal. Hence, checking the terminal type thanks to a call to driverid or even drivername before getting tty control is a pretty good idea.

		By default, all the keypresses will be passed to the client, none will go through brltty, so the application will have to handle console switching itself for instance."""
		cdef unsigned long long code
		if block == 1:
			PY_BEGIN_ALLOW_THREADS
			retval = c_brlapi.brlapi_readKey(block, <unsigned long long*>&code)
			PY_END_ALLOW_THREADS
		else:
			retval = c_brlapi.brlapi_readKey(block, <unsigned long long*>&code)
		if retval == -1:
			raise OperationError(returnerrno())
		elif retval <= 0 and block == False:
			return None
		else:
			return code
	
	def ignoreKeyRange(self, range):
		"""Ignore some key presses from the braille keyboard.

		This function asks the server to give keys between x and y to brltty, rather than returning them to the application via readKey().

		Note: The given codes are either raw keycodes if some driver name was given to getTty(), or brltty commands if None or "" was given."""
		PY_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi_ignoreKeyRange(range[0], range[1])
		PY_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerrno())
		else:
			return retval

	# TODO: ignoreKeySet & unignoreKeySet

	def unignoreKeyRange(self, range):
		"""Unignore some key presses from the braille keyboard.

		This function asks the server to return keys between x and y to the application, and not give them to brltty.

		Note: You shouldn't ask the server to give you key presses which are usually used to switch between TTYs, unless you really know what you are doing ! The given codes are either raw keycodes if some driver name was given to getTty(), or brltty commands if None or "" was given."""
		PY_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi_unignoreKeyRange(range[0], range[1])
		PY_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerror())
		else:
			return retval

	def getRaw(self, drivername):
		"""Switch to Raw mode
		
		* driver : Specifies the name of the driver for which the raw communication will be established"""
		PY_BEGIN_ALLOW_THREADS
		retval = c_brlapi.brlapi_getRaw(drivername)
		PY_END_ALLOW_THREADS
		if retval == -1:
			raise OperationError(returnerror())
		else:
			return retval

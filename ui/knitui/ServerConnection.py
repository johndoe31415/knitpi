#	knitpi - Raspberry Pi interface for Brother KH-930 knitting machine
#	Copyright (C) 2018-2018 Johannes Bauer
#
#	This file is part of knitpi.
#
#	knitpi is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; this program is ONLY licensed under
#	version 3 of the License, later versions are explicitly excluded.
#
#	knitpi is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with knitpi; if not, write to the Free Software
#	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#	Johannes Bauer <JohannesBauer@gmx.de>

import time
import enum
import socket
import struct
import collections
import json

class FileReceptionFailedException(Exception): pass

class ServerConnection(object):
	def __init__(self, socket_filename):
		self._socket_filename = socket_filename
		self._conn = None
		self._error = None

	@property
	def last_error(self):
		return self._error

	def _connect(self):
		if self._conn is None:
			conn = socket.socket(socket.AF_UNIX)
			conn.connect(self._socket_filename)
			self._conn = conn
			self._conn_file = self._conn.makefile(mode = "wrb")

	def _tx(self, command):
		print("->", command)
		self._conn_file.write(command.encode("ascii") + b"\n")

	def _rx(self):
		response = self._conn_file.readline()
		return response

	def _execute(self, command, read_bindata = False, write_bindata = None, parse = False):
		if read_bindata:
			header = self._execute(command, parse = True)
			if (header is None) or (header["msg_type"] != "bindata"):
				raise FileReceptionFailedException("Header invalid for binary file reception.", header)
			bindata = self._conn_file.read(header["bindata_length"])
			return bindata

		try:
			self._connect()
			if write_bindata is None:
				self._tx(command)
			else:
				self._tx(command + " %d" % (len(write_bindata)))
				self._conn_file.write(write_bindata)
			self._conn_file.flush()
			response = self._rx()
			self._error = None
		except (BrokenPipeError, FileNotFoundError, ConnectionRefusedError, OSError) as e:
			self._conn = None
			self._error = e
			response = None
		if parse and (response is not None) and (not read_bindata):
			response = json.loads(response)
		return response

	def get_status(self, parse = False):
		return self._execute("status", parse = parse)

	def get_pattern(self, rawdata = False):
		assert(isinstance(rawdata, bool))
		try:
			return self._execute("getpattern %s" % (str(rawdata)), read_bindata = True)
		except FileReceptionFailedException:
			return None

	def set_pattern(self, xoffset, yoffset, merge, png_data, parse = False):
		assert(isinstance(merge, bool))
		return self._execute("setpattern %d %d %s" % (xoffset, yoffset, str(merge)), write_bindata = png_data, parse = parse)

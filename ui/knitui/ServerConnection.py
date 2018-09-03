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

	def _tx_rx(self, command):
		self._conn_file.write(command.encode("ascii") + b"\n")
		self._conn_file.flush()
		response = self._conn_file.readline()
		return response

	def _get_json(self, command, parse = False):
		try:
			self._connect()
			response = self._tx_rx(command)
			self._error = None
		except (BrokenPipeError, FileNotFoundError, ConnectionRefusedError, OSError) as e:
			self._conn = None
			self._error = e
			response = None
		if parse and (response is not None):
			response = json.loads(response)
		return response

	def get_status(self):
		return self._get_json("status")

	def get_pattern(self):
		pattern_info = self._get_json("getpattern", parse = True)
		if pattern_info is not None:
			png_data = self._conn_file.read(pattern_info["length_bytes"])
			return png_data
		else:
			return b""

if __name__ == "__main__":
	sconn = ServerConnection("../firmware/foo")
	while True:
		print(sconn.get_status())
		print(sconn.get_pattern().hex())
		print(sconn.last_error)
		time.sleep(1)

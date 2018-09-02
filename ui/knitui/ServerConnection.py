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

import enum
import socket
import struct
import collections
import queue


class ServerConnection(object):
	def __init__(self, socket_filename):
		self._socket_filename = socket_filename
		self._conn = None

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

	def _get_json(self, command):
		try:
			self._connect()
			return self._tx_rx(command)
		except (BrokenPipeError, FileNotFoundError, ConnectionRefusedError, OSError) as e:
			print("Exception: %s" % (str(e)))
			return None

	def get_status(self):
		return self._get_json("status")

	def get_pattern(self):
		pattern_info = self._get_json("getpattern")
		print(pattern_info)


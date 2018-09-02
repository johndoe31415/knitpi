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

class CmdCode(enum.IntEnum):
	CMD_GET_STATUS = 0
	RSP_GET_STATUS = 1
	CMD_SET_MODE = 2
	RSP_SET_MODE = 3
	CMD_SET_PATTERN = 4
	RSP_SET_PATTERN = 5

class Message(object):
	HeaderStruct = struct.Struct("< L L")
	HeaderFields = collections.namedtuple("HeaderFields", [ "cmdcode", "payload_size" ])

	StatusStruct = struct.Struct("< H ? ? ? l L l")
	StatusFields = collections.namedtuple("StatusFields", [ "server_mode", "carriage_position_valid", "belt_phase", "direction_left_to_right", "carriage_position", "skipped_needles_cnt", "pattern_row"])

	def __init__(self, cmdcode, payload = b""):
		assert(isinstance(cmdcode, CmdCode))
		assert(isinstance(payload, bytes))
		self._cmdcode = cmdcode
		self._payload = payload

	@property
	def cmdcode(self):
		return self._cmdcode

	@property
	def payload(self):
		return self._payload

	@property
	def status_payload(self):
		print(self)
		return self.StatusFields(*self.StatusStruct.unpack(self.payload))

	def __bytes__(self):
		fields = self.HeaderFields(cmdcode = self._cmdcode, payload_size = len(self._payload))
		header = self.HeaderStruct.pack(*fields)
		return header + self._payload

	def __str__(self):
		return "Msg<%s, %d: %s>" % (self.cmdcode.name, len(self.payload), self.payload.hex())

class ServerConnection(object):
	def __init__(self, socket_filename):
		self._socket_filename = socket_filename
		self._conn = None

	def _connect(self):
		if self._conn is None:
			self._conn = socket.socket(socket.AF_UNIX)
			self._conn.connect(self._socket_filename)

	def _send(self, message):
		data = bytes(message)
		self._conn.send(data)

	def _recv(self, expect_cmdcode):
		header = self._conn.recv(8)
		fields = Message.HeaderFields(*Message.HeaderStruct.unpack(header))
		payload = self._conn.recv(fields.payload_size)
		cmdcode = CmdCode(fields.cmdcode)
		return Message(cmdcode, payload)

	def get_status(self):
		try:
			self._connect()
			self._send(Message(CmdCode.CMD_GET_STATUS))
			rsp = self._recv(CmdCode.RSP_GET_STATUS)
			if rsp is None:
				return None
			return rsp.status_payload
		except (BrokenPipeError, FileNotFoundError) as e:
			print(e)
			return None

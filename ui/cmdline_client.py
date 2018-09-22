#!/usr/bin/python3
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

import sys
import json
import time
from knitui.ServerConnection import ServerConnection
from MultiCommand import MultiCommand

class Actions(object):
	def __init__(self, cmdname, args):
		self._args = args
		self._conn = ServerConnection(self._args.socket)
		handler = getattr(self, "_run_" + cmdname)
		handler()

	def _run_move(self):
		status = self._conn.get_status(parse = True)
		current = status["carriage_position"]
		steps = self._args.pos - current
		if steps == 0:
			step = 0
			steps = 1
		elif steps > 0:
			step = 1
		else:
			step = -1
			steps = abs(steps)
		for i in range(steps):
			current += step
			print(current)
			result = self._conn.mock_command("setpos", current, parse = True)
			time.sleep(0.01)

	def _run_cstatus(self):
		while True:
			self._run_status()
			time.sleep(1)

	def _run_status(self):
		status = self._conn.get_status(parse = True)
		if status is not None:
			print(json.dumps(status, sort_keys = True, indent = 4))
		else:
			print("Last error: %s" % (self._conn.last_error))


	def _run_getpattern(self):
		data = self._conn.get_pattern(rawdata = not self._args.pretty)
		if data is not None:
			print("Received %d bytes." % (len(data)))
			with open(self._args.pngfile, "wb") as f:
				f.write(data)
		else:
			print("Error receving data.")

	def _run_setpattern(self):
		with open(self._args.pngfile, "rb") as f:
			data = f.read()
		result = self._conn.set_pattern(xoffset = self._args.xoffset, yoffset = self._args.yoffset, merge = self._args.merge, png_data = data, parse = True)
		print(json.dumps(result, sort_keys = True, indent = 4))

mc = MultiCommand()
default_socket = "../firmware/socket"

def genparser(parser):
	parser.add_argument("-s", "--socket", metavar = "filename", default = default_socket, help = "Specifies the UNIX socket that the knitcore is found at, defaults to %(default)s.")
	parser.add_argument("pos", type = int, help = "Move the carriage position to the specified position.")
mc.register("move", "Move the carriage to a position", genparser, action = Actions)

def genparser(parser):
	parser.add_argument("-s", "--socket", metavar = "filename", default = default_socket, help = "Specifies the UNIX socket that the knitcore is found at, defaults to %(default)s.")
mc.register("status", "Get the status of the knit machine core", genparser, action = Actions)

def genparser(parser):
	parser.add_argument("-s", "--socket", metavar = "filename", default = default_socket, help = "Specifies the UNIX socket that the knitcore is found at, defaults to %(default)s.")
mc.register("cstatus", "Get the status of the knit machine core continuously", genparser, action = Actions)

def genparser(parser):
	parser.add_argument("-s", "--socket", metavar = "filename", default = default_socket, help = "Specifies the UNIX socket that the knitcore is found at, defaults to %(default)s.")
	parser.add_argument("-p", "--pretty", action = "store_true", help = "Get the pretty image for the pattern instead of the raw one")
	parser.add_argument("pngfile", type = str, help = "PNG file to save pattern to.")
mc.register("getpattern", "Get the current pattern and save as a PNG file", genparser, action = Actions)

def genparser(parser):
	parser.add_argument("-x", "--xoffset", metavar = "pos", type = int, default = 0, help = "X offset to place pattern at. Defaults to %(default)d.")
	parser.add_argument("-y", "--yoffset", metavar = "pos", type = int, default = 0, help = "Y offset to place pattern at. Defaults to %(default)d.")
	parser.add_argument("-m", "--merge", action = "store_true", help = "Merge given pattern file with currently set pattern instead of replacing it.")
	parser.add_argument("-s", "--socket", metavar = "filename", default = default_socket, help = "Specifies the UNIX socket that the knitcore is found at, defaults to %(default)s.")
	parser.add_argument("pngfile", type = str, help = "PNG file to load pattern from.")
mc.register("setpattern", "Set the current pattern to the given PNG file", genparser, action = Actions)

mc.run(sys.argv[1:])

#elif args.command in [ "clr", "center", "trim" ]:
#	result = conn.edit_pattern(args.command, parse = True)
#	print(result)

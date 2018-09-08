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
from FriendlyArgumentParser import FriendlyArgumentParser

parser = FriendlyArgumentParser()
parser.add_argument("--xoffset", metavar = "pixels", type = int, default = 0, help = "X offset in pixels. Defaults to %(default)s.")
parser.add_argument("--yoffset", metavar = "pixels", type = int, default = 0, help = "X offset in pixels. Defaults to %(default)s.")
parser.add_argument("-c", "--command", choices = [ "status", "cont-status", "getpattern", "setpattern", "clr", "center", "trim", "movelr", "moverl" ], default = "status", help = "Command to execute. Can be one of %(choices)s, defaults to %(default)s.")
parser.add_argument("-f", "--file", metavar = "filename", default = "pattern.png", help = "For all operations that require an input/output file, this is the filename. Defaults to %(default)s.")
parser.add_argument("socket", metavar = "file", type = str, help = "Socket to connect to.")
args = parser.parse_args(sys.argv[1:])

conn = ServerConnection(args.socket)
if args.command == "status":
	status = conn.get_status(parse = True)
	print(json.dumps(status, sort_keys = True, indent = 4))
elif args.command == "cont-status":
	while True:
		status = conn.get_status(parse = True)
		if status is not None:
			print(json.dumps(status, sort_keys = True, indent = 4))
		else:
			print("Last error: %s" % (conn.last_error))
		time.sleep(1)
elif args.command == "getpattern":
	data = conn.get_pattern(rawdata = True)
	if data is not None:
		print("Received %d bytes." % (len(data)))
		with open(args.file, "wb") as f:
			f.write(data)
	else:
		print("Error receving data.")
elif args.command == "setpattern":
	with open(args.file, "rb") as f:
		data = f.read()
	result = conn.set_pattern(xoffset = args.xoffset, yoffset = args.yoffset, merge = False, png_data = data)
	print(result)
elif args.command in [ "clr", "center", "trim" ]:
	result = conn.edit_pattern(args.command, parse = True)
	print(result)
elif args.command in [ "movelr", "moverl" ]:
	rng = range(50, 150 + 1)
	if args.command == "moverl":
		rng = reversed(rng)
	for i in rng:
		result = conn.mock_command("setpos", i, parse = True)
		print(result)
		time.sleep(0.01)
else:
	raise NotImplementedError(args.command)

if conn.last_error is not None:
	print("Last error: %s" % (conn.last_error))


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
import random
import json
import flask
import mako.lookup
from knitui.ServerConnection import ServerConnection

class Controller(object):
	def __init__(self):
		self._config = {
			"server_socket":		"../firmware/foo",
		}
		self._template_lookup = mako.lookup.TemplateLookup([ "knitui/templates" ], input_encoding = "utf-8", strict_undefined = True)

	def _serve(self, template_name, args = None):
		if args is None:
			args = { }
		template = self._template_lookup.get_template(template_name)
		result = template.render(**args)
		return result

	def debug(self, request):
		return self._serve("debug.html")

	def index(self, request):
		return self._serve("index.html")

	def rest_pattern_get(self, request):
		server_connection = ServerConnection(self._config["server_socket"])
		pattern = server_connection.get_pattern()
		if len(pattern) > 0:
			return flask.Response(pattern, mimetype = "image/png")
		else:
			return flask.Response("No pattern loaded.\n", status = 404, mimetype = "text/plain")

	def ws_status(self, ws):
		server_connection = ServerConnection(self._config["server_socket"])
		while True:
			status_json = server_connection.get_status()
			if status_json is not None:
				ws.send(status_json)
			else:
				msg = {
					"msg_type":		"connection_error",
					"text":			str(server_connection.last_error),
				}
				ws.send(json.dumps(msg))
				time.sleep(1)
			time.sleep(0.25)

	def ws_echo(self, ws):
		ws.send(b"Welcome to the Echo server")
		while True:
			msg = ws.receive()
			if msg is not None:
				if msg != b"":
					ws.send(b"Echoed(" + msg + b")")
			else:
				break

	def ws_push(self, ws):
		ws.send(b"Welcome to the Push server")
		while True:
			rnd = random.random()
			msg = "Waiting %.3f secs" % (rnd)
			ws.send(msg.encode())
			time.sleep(rnd)

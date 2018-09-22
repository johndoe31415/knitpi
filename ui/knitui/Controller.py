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
import threading
import mako.lookup
import subprocess
from knitui.ServerConnection import ServerConnection
import gevent.event

class Controller(object):
	def __init__(self, config):
		self._config = config
		self._template_lookup = mako.lookup.TemplateLookup([ self._config["template_lookup_dir"] ], input_encoding = "utf-8", strict_undefined = True)
		self._isleep_cond = gevent.event.Event()

	def _handler_system(self, template_args):
		template_args["firmware_version"] = "Unavailable"
		template_args["firmware_date"] = "Unavailable"
		try:
			template_args["firmware_version"] = subprocess.check_output([ "git", "describe", "--abbrev=10", "--dirty", "--always" ]).decode("utf-8")
			template_args["firmware_date"] = subprocess.check_output([ "git", "log", "-1", "--format=%cd", "--date=iso" ]).decode("utf-8")
		except (FileNotFoundError, subprocess.CalledProcessError, UnicodeDecodeError) as e:
			pass

		template_args["temperature_exception"] = None
		try:
			temp = subprocess.check_output([ "vcgencmd", "measure_temp" ])
			temp = temp.decode("utf-8").rstrip("\r\n")
			if temp.startswith("temp="):
				temp = temp[5:]
			if temp.endswith("'C"):
				temp = temp[:-2] + "°C"
		except (FileNotFoundError, subprocess.CalledProcessError, UnicodeDecodeError) as e:
			temp = "Unavailable"
			template_args["temperature_exception"] = str(e)
		template_args["temperature"] = temp

		template_args["hwinfo"] = None
		server_connection = ServerConnection(self._config["server_socket"])
		template_args["hwinfo"] = server_connection.get_hwinfo(parse = True)

	def serve_page(self, request, template_name, args = None):
		if args is None:
			args = { }
		try:
			template = self._template_lookup.get_template(template_name + ".html")
		except mako.exceptions.TopLevelLookupException:
			return flask.Response("Template '%s' not found.\n" % (template_name), status = 404, mimetype = "text/plain")
		additional_data_handler = getattr(self, "_handler_" + template_name, None)
		if additional_data_handler is not None:
			additional_data_handler(args)
		result = template.render(**args)
		return result

	def rest_pattern_get(self, request, raw_pattern = False, attachment = False):
		server_connection = ServerConnection(self._config["server_socket"])
		pattern = server_connection.get_pattern(rawdata = raw_pattern)
		if pattern is not None:
			headers = { }
			if attachment:
				status = server_connection.get_status(parse = True)
				filename = "knitpi_pattern_%03dx%03d.png" % (status["pattern_width"], status["pattern_height"])
				headers["Content-Disposition"] = "attachment; filename=\"%s\"" % (filename)
			return flask.Response(pattern, mimetype = "image/png", headers = headers)
		else:
			return flask.Response("No pattern loaded.\n", status = 404, mimetype = "text/plain")

	def _msg(self, request, msgtype, msg):
		# TODO: Implement me!
		print(msgtype, msg)

	def rest_pattern_post(self, request):
		msg = None
		if ("set_pattern" in request.form) or ("merge_pattern" in request.form):
			if "pattern" not in request.files:
				self._msg(request, "error", "No file provided.")
			else:
				try:
					(x, y) = (int(request.form["xoffset"]), int(request.form["yoffset"]))
				except ValueError:
					self._msg(request, "error", "Could not read X or Y offset.")

				pattern_file = request.files["pattern"]
				if pattern_file.mimetype not in [ "image/png" ]:
					self._msg(request, "error", "Unsupported file type uploaded: %s" % (pattern_file.mimetype))
				else:
					file_data = pattern_file.stream.read()
					server_connection = ServerConnection(self._config["server_socket"])
					server_connection.set_pattern(xoffset = x, yoffset = y, merge = ("merge_pattern" in request.form), png_data = file_data)
					if server_connection.last_error is not None:
						self._msg(request, "error", "Could not send pattern to knitserver.")
		elif "center_pattern" in request.form:
			self.rest_editpattern(request, "center")
		elif "trim_pattern" in request.form:
			self.rest_editpattern(request, "trim")
		elif "clr_pattern" in request.form:
			self.rest_editpattern(request, "clr")
		else:
			self._msg(request, "error", "Unknown request sent to rest_pattern_post.")
		return flask.redirect("/page/pattern")

	def _json_response(self, response):
		return flask.Response(str(response), status = 200, mimetype = "application/json")

	def _single_server_action(self, action):
		server_connection = ServerConnection(self._config["server_socket"])
		result = action(server_connection)
		self._isleep_interrupt()
		return result

	def rest_hwinfo(self, request):
		server_connection = ServerConnection(self._config["server_socket"])
		return flask.Response(server_connection.get_hwinfo(), status = 200, mimetype = "application/json")

	def rest_setrow(self, request, rowid):
		return self._single_server_action(lambda server_connection: server_connection.set_row(rowid))

	def rest_setpatternoffset(self, request, new_offset):
		return self._single_server_action(lambda server_connection: server_connection.set_offset(new_offset))

	def rest_setknittingmode(self, request, knitting_mode):
		return self._single_server_action(lambda server_connection: server_connection.set_knitting_mode(knitting_mode))

	def rest_setrepeatmode(self, request, repeat_mode):
		return self._single_server_action(lambda server_connection: server_connection.set_repeat_mode(repeat_mode))

	def rest_editpattern(self, request, edit_type):
		return self._single_server_action(lambda server_connection: server_connection.edit_pattern(edit_type))

	def rest_firmware_update(self, request):
		try:
			subprocess.check_call([ "sudo", "/bin/systemctl", "start", "knitpi-update" ])
		except subprocess.CalledProcessError as e:
			return flask.Response(json.dumps({ "status": "error", "message": str(e) }) + "\n", status = 400, mimetype = "application/json")
		return flask.Response(json.dumps({ "status": "ok", "message": "Triggered firmware update." }) + "\n", status = 200, mimetype = "application/json")

	def _isleep(self, max_sleeptime):
		self._isleep_cond.clear()
		self._isleep_cond.wait(timeout = max_sleeptime)

	def _isleep_interrupt(self):
		self._isleep_cond.set()

	def ws_status(self, ws):
		server_connection = ServerConnection(self._config["server_socket"])
		wait_milliseconds = 0
		while ws.connected:
			status_json = server_connection.get_status(wait_milliseconds = wait_milliseconds)
			if status_json is not None:
				ws.send(status_json)
			else:
				msg = {
					"msg_type":		"connection_error",
					"text":			str(server_connection.last_error),
				}
				ws.send(json.dumps(msg))

			# Limit minimum wait as well
			self._isleep(0.1)

			# First status requested immediately, updates only on demand, but at least once a second
			wait_milliseconds = 1000

	def ws_echo(self, ws):
		ws.send(b"Welcome to the Echo server")
		while ws.connected:
			msg = ws.receive()
			if msg is not None:
				if msg != b"":
					ws.send(b"Echoed(" + msg + b")")
			else:
				break

	def ws_push(self, ws):
		ws.send(b"Welcome to the Push server")
		while ws.connected:
			rnd = random.random()
			msg = "Waiting %.3f secs" % (rnd)
			ws.send(msg.encode())
			time.sleep(rnd)

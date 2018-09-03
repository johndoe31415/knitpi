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

import flask
from knitui import app, websocket
from knitui.Controller import Controller

ctrlr = Controller()

@app.route("/")
def index():
	return ctrlr.serve_page(flask.request, "index")

@app.route("/page/<name>")
def show_page(name):
	return ctrlr.serve_page(flask.request, name)

@app.route("/rest/pattern", methods = [ "GET" ])
def rest_pattern_get():
	return ctrlr.rest_pattern_get(flask.request)

@app.route("/rest/pattern", methods = [ "POST" ])
def rest_pattern_post():
	return ctrlr.rest_pattern_post(flask.request)

@websocket.route("/ws/status")
def ws_status(ws):
	return ctrlr.ws_status(ws)

@websocket.route("/debug/ws-echo")
def ws_echo(ws):
	return ctrlr.ws_echo(ws)

@websocket.route("/debug/ws-push")
def ws_push(ws):
	return ctrlr.ws_push(ws)

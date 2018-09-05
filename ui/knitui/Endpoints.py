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

@app.route("/rest/edit_pattern/<edit_type>", methods = [ "POST" ])
def rest_editpattern_post(edit_type):
	return ctrlr.rest_edit_pattern(flask.request, edit_type)

@app.route("/rest/pattern_row/<int:rowid>", methods = [ "POST" ])
def rest_setrow_post(rowid):
	return ctrlr.rest_setrow(flask.request, rowid)

@app.route("/rest/pattern_offset/<new_offset>", methods = [ "POST" ])
def rest_pattern_offset_post(new_offset):
	new_offset = int(new_offset)
	return ctrlr.rest_setpatternoffset(flask.request, new_offset)

@app.route("/rest/set_knitting_mode/<knitting_mode>", methods = [ "POST" ])
def rest_setknittingmode_post(knitting_mode):
	return ctrlr.rest_setknittingmode(flask.request, knitting_mode)

@app.route("/rest/set_repeat_mode/<repeat_mode>", methods = [ "POST" ])
def rest_setrepeatmode_post(repeat_mode):
	return ctrlr.rest_setrepeatmode(flask.request, repeat_mode)

@websocket.route("/ws/status")
def ws_status(ws):
	return ctrlr.ws_status(ws)

@websocket.route("/debug/ws-echo")
def ws_echo(ws):
	return ctrlr.ws_echo(ws)

@websocket.route("/debug/ws-push")
def ws_push(ws):
	return ctrlr.ws_push(ws)

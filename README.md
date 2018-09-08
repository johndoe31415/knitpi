# knitpi
![I love KnitPi](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/logo.png)
![Picture of KnitPi](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/overview.jpg)

KnitPi is a Raspberry Pi interface for a Brother KH-930 knitting machine. It's
functional, but very experimental.

## Installation
Installation on a Raspberry Pi is very easy thanks to the "install_locally"
script that is provided. Copy it to your Raspberry Pi and execute it as root.
E.g.:

```
reliant joe $ scp install_locally root@192.168.1.123:
reliant joe $ ssh root@192.168.1.123
knitpi root # ./install_locally
[...]
```

The script will do the following:

  * Install all dependencies via apt-get.
  * Create a "knitpi" user with the appropriate permissions and a /home/knitpi
    home directory.
  * Clone the knitpi source files directly off GitHub, compile the knitting core.
  * Install systemd services for both the core and the UI and start them.

This means, once you run the install script, everything should work, starting
from a fresh Raspbian image. I've tested it with
2018-06-27-raspbian-stretch-lite.img -- if there's anything that doesn't work
with future versions, please report it as a bug.

After the Pi has booted, it should now serve a webserver at port 80. I.e., you
can browse to http://192.168.1.123 and should see the KnitPi UI.

## Software
The software runs directly on the Raspberry Pi. The core is written in C to get
as low latency as possible. So far, we've not encountered issues regarding
timing (e.g., lost rotary encoder ticks and such). It seems to work well
without the need to use an external microcontroller, which should make
replicating this project extremely simple.


## Dependencies
KnitPi does not rely on any Raspberry Pi specific hardware, but I run it on a
Pi 3+. It only relies on the Linux kernel GPIO and SPI drivers and so should be
portable to any kind of embedded Linux system. For Raspbian, you can install
the required header files using:

```
# apt-get install libgpiod-dev
```

To run the UI, you'll need uwsgi and its Python3 plugin, gevent as well as
Flask and Mako:

```
# apt-get install uwsgi uwsgi-plugin-python3 python3-gevent python3-flask python3-mako
```

## Hardware
I've build a custom Raspberry Pi shield on prototype board from scratch. The
schematics [are in the Git
repository](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/KnittingPi.pdf).
I've drawn them with CircuitMaker and the [CircuitMaker project page can be
found here](https://circuitmaker.com/Projects/Details/johndoe31415/KnittingPi).


## Included third-party code/data
knitpi uses third-party code that is included in the tree for ease of use. In
particular, it uses:

  * The [flask_uwsgi_websocket module of Zach Kelling / @zeekay](https://github.com/zeekay/flask-uwsgi-websocket). 
    It is licensed under the MIT license.
  * The [pure.css framework](https://github.com/pure-css/pure). It is licensed
    under the Yahoo! Inc BSD license.
  * The ["pattern finished" chime](https://freesound.org/people/JFRecords/sounds/420512/)
	is licensed under the CC-BY 3.0 license and was downloaded from
	[freesound.org](https://www.freesound.org). It was created by
	[JFRecords](https://freesound.org/people/JFRecords/) under the original
    name "vMax Bells".

My sincere thanks go out to both of these.

## License
All my code is licensed under the GNU GPL-3. All included code is licensed
under their respective licenses which are included in the 3rdparty
subdirectory.

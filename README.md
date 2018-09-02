# knitpi
![I love KnitPi](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/logo.png)

KnitPi is a Raspberry Pi interface for a Brother KH-930 knitting machine. It's
functional, but very experimental.


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

To run the UI, you'll need uwsgi and its Python3 plugin:

```
# apt-get install uwsgi uwsgi-plugin-python3
```

## Hardware
I've build a custom Raspberry Pi shield on prototype board from scratch. The
schematics [are in the Git
repository](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/KnittingPi.pdf).
I've drawn them with CircuitMaker and the [CircuitMaker project page can be
found here](https://circuitmaker.com/Projects/Details/johndoe31415/KnittingPi).


## Included third-party code
knitpi uses third-party code that is included in the tree for ease of use. In
particular, it uses:

  * The [flask_uwsgi_websocket module of Zach Kelling / @zeekay](https://github.com/zeekay/flask-uwsgi-websocket). 
    It is licensed under the MIT license.
  * The [pure.css framework](https://github.com/pure-css/pure). It is licensed
    under the Yahoo! Inc BSD license.

My sincere thanks go out to both of these.

## License
All my code is licensed under the GNU GPL-3. All included code is licensed
under their respective licenses which are included in the 3rdparty
subdirectory.

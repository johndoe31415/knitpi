# knitpi
KnitPi is a Raspberry Pi interface for a Brother KH-930 knitting machine. It's
not yet functional, this is very early prototyping work.

## Dependencies
KnitPi does not rely on any Raspberry Pi specific hardware, but I run it on a
Pi 3+. It only relies on the Linux kernel GPIO and SPI drivers and so should be
portable to any kind of embedded Linux system. For Raspbian, you can install
the required header files using:

```
# apt-get install libgpiod-dev
```

## License
GNU GPL-3.

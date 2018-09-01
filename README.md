# knitpi
![I love KnitPi](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/logo.png)

KnitPi is a Raspberry Pi interface for a Brother KH-930 knitting machine. It's
functional, but very experimental.

## Software Dependencies
KnitPi does not rely on any Raspberry Pi specific hardware, but I run it on a
Pi 3+. It only relies on the Linux kernel GPIO and SPI drivers and so should be
portable to any kind of embedded Linux system. For Raspbian, you can install
the required header files using:

```
# apt-get install libgpiod-dev
```

## Hardware
I've build a custom Raspberry Pi shield on prototype board from scratch. The
schematics [are in the Git
repository](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/KnittingPi.pdf).
I've drawn them with CircuitMaker and the [CircuitMaker project page can be
found here](https://circuitmaker.com/Projects/Details/johndoe31415/KnittingPi).

## License
GNU GPL-3.

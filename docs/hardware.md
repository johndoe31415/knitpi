# Brother KH-930 Hardware Documentation
This page documents the hardware interface of the KH-930 (and, likely, also
applies to similar knitting machines). Much of this has been found out by
reverse engineering because much of the information in the service manual was
not detailed enough (for my taste) and many of other OSS projects which
interface knitting machines don't document this either.


## Supply voltage
The KH-930 has a transformer with two distinct windings, one for the supply
(originally with a 2.5A fuse) and the other one for the solenoid actuators
(originally with a 5A fuse).  Those ratings seem very high for modern (SMPS)
switching. I've replaced both rails with a simple bridge rectifier followed by
a SMPS module and regulate the digital portion to 5V, the mechanical portion to
12V. Regulation isn't strictly necessary for the solenoid port, but it really
isn't harmful either.


## Electrical Connections
Inputs/outputs are described from the knitting machine side:

  * In: 16 x solenoid inputs. These are power inputs that source current. About
    100mA at 12V for each individual one. To the best of my knowledge, they are
    designed to operate at a 100% duty cycle. They can be easily driven with a
    ULN200x or ULN280x or discretely via e.g., a BC338.
  * Out: 2 x hall sensor analog outputs: They give out an analog voltage of 0..5V.
  * Out: Rotary encoder V1/V2 signal. Standard 5V TTL digital signal that
    encodes relative movement of the carriage position.
  * Out: BP signal. Gives the belt phase. There are two ways the carriage can
    interlock with the belt, offset by 8 needles. You need to measure the BP
    signal while the carriage is over the left or right hall sensor fixed position
    to determine the current belt phase and offset the solenoid actuation by the
    correct offset.


## Operation of the Knitting Machine
It's actually very simple: Say you always actuate one solenoid without looking
at any rotary encoder inputs, then every 16th needle will be set. Depending on
the belt phase, this means that all needles (16 * i + 0) or all needles (16 * i
+ 8) will be set.

Solenoids will have to be actuated *after* the carriage moves over them and the
solenoids need to remain engaged for a while. I found 12 out of 16 results in
reliable operation.

The following diagram shows what you would need to do when you only wanted to
engage needle "yellow 16". The blue numbers give a zero-based continuous
offset, i.e., yellow 16 is #84.

First, determine the correct solenoid: The corresponding solenoid for needle #84
is Solenoid 5 (84 mod 16 = 5) if the belt phase is 0. Otherwise, when the belt
phase is 1, Solenoid 13 is correct ((84 + 8) mod 16 = 13).

Secondly, determine the window in which solenoid 5 or 13 has to be engaged.
Note, as shown in the diagram, that the engagement needs to happen *after* the
rotary encoder indicates that the needle has passed. I.e., when moving from
left to right, the solenoid has to be actuated starting from needle #96 (yellow
4) up until needle #107 (green 8). Moving in the opposite direction, i.e., from
right to left, the solenoid has to be turned on starting from needle #71
(yellow 29) up until needle #60 (yellow 40).

Moving from left to right, the windows for each needle therefore are:

window_lo = needle_pos + 12
window_hi = needle_pos + 23

Moving from right to left, they are:

window_lo = needle_pos - 24
window_hi = needle_pos - 13

However, in normal operation we have the opposite situation: We want to figure
out for a given carriage position which the minimal and maximal needle is that
we need to actuate (if they are set). This is also pretty easy. Moving from
left to right:

min_needle_pos = carriage_pos - 23
max_needle_pos = carriage_pos - 12

And from right to left:

min_needle_pos = carriage_pos - 24
max_needle_pos = carriage_pos - 13


![Needle actuation diagram](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/needle_actuation.svg)

TODO: double check if this is accurate. Seems to have a off-by-one?


In summary, the operation works as follows:

  * Wait for the carriage to move over the left or right sensor. Calibrate
    known position at 0 (left side) or 200 (right side). Determine belt phase
    while over hall sensor.
  * Record carriage position by interpreting the relative rotary encoder inputs
    V1/V2.
  * Depending on the determined carriage position, determine the "active"
    needle window (min_needle_pos to max_needle_pos).
  * Determine which of the needles in this window need to be set (depending on
    the pattern).
  * Determine the corresponding solenoids (mind the belt phase) and actuate them.


## Hall sensor signal conditioning
There are three different types of carriages (knitting, lace and automatic)
and, unfortunately, they deliberately create differen analog signals (so the
equipment can determine which sled is used). I've just implemented one type of
sled (knitting), therefore for my use case I've just used a simple comparator
circuit and set the threshold values by experimentation so the digital signal
worked. I think this isn't a bad approach and if I had to add support for more
carriages, I'd simply add more comparators and configure the thresholds to be
able to distinguish the voltage levels properly.


## Operation of KnitPi
KnitPi works very straightforward: It uses a serial-to-parallel shift register
(74HC595) to convert a 2-byte SPI output from the Raspberry Pi to 16 0..3.3V
digital outputs. Each of these are amplified by a set of ULN2801. I wasn't sure
of a ULN2803 would work with 3.3V inputs, but that would certainly simplify the
circuit (you could leave out the 1k series base resistors then).

The digital outputs are converted using a CD4050, which is 5V-tolerant and
operated from 3.3V. Then the V1, V2 and BP digital outputs are directly fed to
GPIO inputs of the Pi.

For the hall sensor input, the signal is pre-processed by a LM393 comparator.
It's open-collector, so adaption to the 3.3V output is really easy. The output
signal is then also fed into the GPIO inputs of the Pi.

The main control of KnitPi is done by the "core" or "firmware". It's a C
program that instruments the low-level hardware interface.  It opens up a UNIX
socket that it listens for commands on. You can connect to that socket and
control, for example, the mode of operation or the pattern that should be knit.
Debugging is fairly easy, just connect via socat:

```
$ socat - unix-connect:/home/knitpi/socket
```

Then try typing a command like "status" and hitting Enter:

```
status
{ "msg_type": "status", "knitting_mode": false, "repeat_mode": "oneshot", "carriage_position_valid": false, "even_rows_left_to_right": false, "carriage_position": 0, "skipped_needles_cnt": 0, "pattern_row": 0, "pattern_offset": 0, "pattern_min_x": 0, "pattern_min_y": 0, "pattern_max_x": -1, "pattern_max_y": -1, "pattern_width": 0, "pattern_height": 0 }
```

The UI connects to this UNIX socket and passes some of the messages to the
client via a WebSocket connection.

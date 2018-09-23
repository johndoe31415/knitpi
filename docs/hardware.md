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

  * *In*: 16 x solenoid inputs. These are power inputs that source current. About
    100mA at 12V for each individual one. To the best of my knowledge, they are
    designed to operate at a 100% duty cycle. They can be easily driven with a
    ULN200x or ULN280x or discretely via e.g., a BC338.
  * *Out*: 2 x hall sensor analog outputs: They give out an analog voltage of 0..5V.
  * *Out*: Rotary encoder V1/V2 signal. Standard 5V TTL digital signal that
    encodes relative movement of the carriage position.
  * *Out*: BP signal. Gives the belt phase. There are two ways the carriage can
    interlock with the belt, offset by 8 needles. You need to measure the BP
    signal while the carriage is over the left or right hall sensor fixed position
    to determine the current belt phase and offset the solenoid actuation by the
    correct offset.


## Operation of the Knitting Machine
It's actually very simple: Say you always actuate one solenoid without looking
at any rotary encoder inputs, then every 16th needle will be set. Depending on
the belt phase, this means that all needles (16 * i + 0) or all needles (16 * i
+ 8) will be set.

Now when you try to get the correct one, you will have to time the actuation
according to the carriage position which you determined from the rotary
encoders. Important here is that the solenoids have to be actuated *after* the
carriage moves over them and that the solenoids need to remain engaged for a
while. I.e., there is a "window" of needles which are active that I call the
"actuation window". Therefore, there are two parameters that have to be tuned:
The offset of the window to the carriage position and the window size. I've
experimented with this a bit and am currently using a setup of offset 11 and
window size 16 (the maximum) which seems to produce reliable results. With a
window size of 16, any offset less than 6 and any offset more than 16 resulted
in unreliable operation -- therefore I opted for the average value of the two
(11).

The following diagram shows what needles are in the actuation window when the
carriage position is hovering over "green 65". It depens on the movement
direction: If the carriage is moving from left-to-right, then the needles
"green 39" to "green 54" are active in this case. Moving right-to-left, needles
"green 76" to "green 91" are the "hot" ones.

![Needle actuation diagram](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/needle_actuation.svg?sanitize=true)

Once you know what needles are in the actuation window, you need to determine
for each of these needles (16 in my case) if you want them set or not. For
those that you want set, you need to engage the solenoids.  So for example,
let's say we're moving left-to-right and we want to set only three needles:
"green 40", "green 50" and "green 60".

At position "green 64" with left-to-right movement, the actuation window goes
from "green 39" to "green 54". Therefore, we only have two needles to consider:
"green 40" and "green 50" -- "green 60" is not in the window and can be
ignored.

We therefore have to engage two solenoids. Let's say the belt phase is 0. Then
determining the correct ones is easy:

```
belt phase 0:
green 40 = 139 -> 139 % 16 = 11
green 50 = 149 -> 149 % 16 = 5
```

Therefore, engage solenoids 5 and 11 at this carriage position to get the
desired result. When the belt phase is 1, it's only slightly more complicated:

```
belt phase 1:
green 40 = 139 -> (139 + 8) % 16 = 3
green 50 = 149 -> (149 + 8) % 16 = 13
```

For belt phase 1, we therefore need solenoids 3 and 13 to engage.

It may be a bit complicated, but the following image shows all solenoids and at
which time they need to be active to get a specific pattern:

![Movement left-to-right](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/move_left_to_right.png)

Every needle is 4x4 pixels wide. You see on the topmost row (in black) the
pattern that you want to knit. Then, below, there are 16 rows (each also 4
pixels in height) which indicate all 16 solenoids. They're colored in whenever
they need to be engaged.

Consequently, when moving right-to-left for the exact same pattern, here's the
timings that they need to be engaged at:

![Movement right-to-left](https://raw.githubusercontent.com/johndoe31415/knitpi/master/docs/move_right_to_left.png)

In summary, the operation works as follows:

  * Wait for the carriage to move over the left or right sensor. Calibrate
    known position at 0 (left side) or 200 (right side). Determine belt phase
    while over hall sensor.
  * Record carriage position by interpreting the relative rotary encoder inputs
    V1/V2.
  * Depending on the determined carriage position, determine the "hot" needle
    window.
  * Determine which of the needles in this "hot" actuation window need to be
    set (depending on the pattern).
  * Determine the corresponding solenoids (considering the belt phase) and
    actuate them.


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

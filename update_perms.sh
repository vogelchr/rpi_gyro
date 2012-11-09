#!/bin/sh
set -x -v

chmod 666 /dev/spidev0*
chmod 666 /sys/class/gpio/gpio25/direction
chmod 666 /sys/class/gpio/gpio25/value


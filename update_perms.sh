#!/bin/sh
set -x -v

chmod 666 /dev/spidev0*
if ! [ -d /sys/class/gpio/gpio25 ]  ; then
	echo 25 >/sys/class/gpio/export
fi
chmod 666 /sys/class/gpio/gpio25/direction
chmod 666 /sys/class/gpio/gpio25/value


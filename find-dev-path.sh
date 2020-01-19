#!/bin/bash --
#
# Prints the device path for each connected device (so we can
# look up which one is the Arduino - if we forgot to query
# dev paths before)
#
# Taken from https://unix.stackexchange.com/a/144735

for sysdevpath in $(find /sys/bus/usb/devices/usb*/ -name dev); do
    (
        syspath="${sysdevpath%/dev}"
        devname="$(udevadm info -q name -p $syspath)"
        [[ "$devname" == "bus/"* ]] && continue
        eval "$(udevadm info -q property --export -p $syspath)"
        [[ -z "$ID_SERIAL" ]] && continue
        echo "/dev/$devname - $ID_SERIAL"
    )
done

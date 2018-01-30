#!/bin/bash
set -e

# Sync executable
bn=$(basename $1)
rsync -az $1/build/tp2serveur pi@$2:/home/pi/projects/laboratoire2/tp2serveur

# Execute GDB
ssh pi@$2 "nohup gdbserver :4567 /home/pi/projects/laboratoire2/tp2serveur > /dev/null 2> /dev/null < /dev/null &"
sleep 1
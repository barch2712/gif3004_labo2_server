#!/bin/bash
set -e

# Sync executable
bn=$(basename $1)
rsync -az $1/build/tp2client pi@$2:/home/pi/projects/laboratoire2/tp2client

# Execute GDB
ssh pi@$2 "nohup gdbserver :4568 /home/pi/projects/laboratoire2/tp2client -f -s /home/pi/projects/laboratoire2/pointdemontage > /dev/null 2> /dev/null < /dev/null &"
sleep 1
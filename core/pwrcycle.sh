#!/bin/bash

# Usage: ./pwrcycle.sh <pin_number> <duration_seconds>

PIN=$1
DURATION=$2

if [ -z "$PIN" ] || [ -z "$DURATION" ]; then
          echo "Usage: $0 <pin_number> <duration_seconds>"
            exit 1
fi

# Turn on GPIO
gpioset gpiochip0 $PIN=1

# Sleep for the duration
sleep $DURATION

# Turn off GPIO
gpioset gpiochip0 $PIN=0

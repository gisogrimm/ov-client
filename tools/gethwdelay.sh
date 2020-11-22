#!/bin/bash
CARD="$1"
RATE="$2"
PERIODSIZE="$3"
INPUTPORT="$4"
OUTPUTPORT="$5"
if [ "$#" -ne 5 ]; then
    echo "Usage: ./gethwdelay.sh CARD SRATE PERIODSIZE INPUTPORT OUTPUTPORT"
    echo ""
    echo " - CARD : ALSA sound card name, e.g., USB"
    echo " - SRATE : Sampling rate in Hz, e.g. 48000"
    echo " - PERIODSIZE : Jack period size, e.g., 96"
    echo " - INPUTPORT : Hardware input channel to which the cable is connected"
    echo " - OUTPUTPORT : Hardware output channel to which the cable is connected"
    echo ""
    echo "Example:"
    echo "./gethwdelay.sh USB 48000 96 2 2"
    echo ""
    echo "Measures delay of the USB sound card at 48 kHz and 96 samples per period."
    echo "An analog cable is connected from second output to second input."
    echo ""
    echo "Detected sound cards:"
    cat /proc/asound/cards
    exit
fi
killall jackd jack_delay 2>/dev/null
jackd --sync -P 90 -d alsa -d hw:$CARD -r $RATE -p $PERIODSIZE >/dev/null 2>/dev/null &
JACKPID=$!
sleep 2

jack_delay -I system:capture_$INPUTPORT -O system:playback_$OUTPUTPORT > delayout 2>/dev/null &
DELPID=$!
sleep 3
kill $DELPID
sleep 1
kill $JACKPID
cat delayout | sort -u | tail -1

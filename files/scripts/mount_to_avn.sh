#!/bin/sh

AVN_CONNECTED=$(ls /var/lib/docker/overlay2/ | wc -l)
if [ $AVN_CONNECTED == 7 ]
then
    CONTAINER_AVM=$(ls /var/lib/docker/overlay2/ | tail -3 | head -1)
    echo "AVN: $CONTAINER_AVN"
    mkdir /var/lib/docker/overlay2/$CONTAINER_AVN/merged/tmp/usb_avn
    mkdir /var/lib/docker/overlay2/$CONTAINER_AVN/merged/tmp/usb_avn/$1
    DRIVE="/dev/"$1
    mount $DRIVE /var/lib/docker/overlay2/$CONTAINER_AVN/merged/tmp/usb_avn/$1
fi

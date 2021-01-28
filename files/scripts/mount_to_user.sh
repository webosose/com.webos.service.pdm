#!/bin/sh

MNT_DIR="usb_"$1
mkdir /tmp/$MNT_DIR/$2
USER="$1"
DRIVE="/dev/"$2
mount $DRIVE /tmp/$MNT_DIR/$2

chown $1 /tmp/$MNT_DIR/$2
chgrp $1 /tmp/$MNT_DIR/$2
chmod 770 /tmp/$MNT_DIR/$2

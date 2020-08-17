#!/bin/sh

# Check the number of displays connected
DISPLAYS_CONNECTED=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep deviceSetId | awk '{print $2}' | tr -d ',"' | wc -l)
# If 2, then AVN is not connected
if [ $DISPLAYS_CONNECTED == 2 ]
then
    # Check if the first deviceSetId is "RSE-L" and get the corresponding sessionId
    DEVICE_SET_ID=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep deviceSetId | awk '{print $2}' | tr -d ',"' | head -1)
    if [ "$DEVICE_SET_ID" == "RSE-L" ]
    then
        SESSION_ID=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep sessionId | awk '{print $2}' | tr -d ',"' | head -1)
    else
        SESSION_ID=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep sessionId | awk '{print $2}' | tr -d ',"' | tail -1)
    fi
# If 3, then AVN is connected
elif [ $DISPLAYS_CONNECTED == 3 ]
then
    DEVICE_SET_ID1=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep deviceSetId | awk '{print $2}' | tr -d ',"' | head -1)
    DEVICE_SET_ID2=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep deviceSetId | awk '{print $2}' | tr -d ',"' | tail -2 | head -1)
    if [ "$DEVICE_SET_ID1" == "RSE-L" ]
    then
        SESSION_ID=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep sessionId | awk '{print $2}' | tr -d ',"' | head -1)
    elif [ "$DEVICE_SET_ID2" == "RSE-L" ]
    then
        SESSION_ID=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep sessionId | awk '{print $2}' | tr -d ',"' | tail -2 | head -1)
    else
        SESSION_ID=$(luna-send -n 1 -f luna://com.webos.service.sessionmanager/getSessionList '{}' | grep sessionId | awk '{print $2}' | tr -d ',"' | tail -1)
    fi
fi
CONTAINER_L=$(docker inspect "$SESSION_ID" | grep MergedDir | awk '{print $2}' | tr -d '",')
echo "RSE-L: $CONTAINER_L"
mkdir $CONTAINER_L/tmp/usb_rse_left
mkdir $CONTAINER_L/tmp/usb_rse_left/$1
DRIVE="/dev/"$1
mount $DRIVE $CONTAINER_L/usb_rse_left/$1

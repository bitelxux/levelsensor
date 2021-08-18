#!/bin/bash

BASEDIR=/root/sensor.agua

ls $BASEDIR/alive > /dev/null && find . -mmin -5 -type f -print | grep alive || (killall -TERM server.py && echo "Terminado")

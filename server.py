#!/usr/bin/python

import os
import random
import serial
import time
import re
from datetime import date, datetime

BASEDIR = "/root/sensor.agua"
MAX = 1300

rParser = re.compile(r"d: (?P<distancia>.*) h: (?P<altura>.*) v: (?P<litros>\d*)")

last_value = None

logfile = open("agua.log", "aw")
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.1)

os.system("rm -f %s/alive" % BASEDIR)


def store_reading(value):
    with open("%s/lecturas/%d" % (BASEDIR, time.time()), "w") as f:
        f.write(str(value))

if __name__ == "__main__":
    counter = 0

    while True:
        line = ser.readline()   # read a '\n' terminated line
        if line:
            print(">>> %s %s" % (counter, line[:-1]))
            counter += 1
            now = time.strftime("%D %H:%M:%S", time.localtime(time.time()))
            text = "%s %s" % (now, line)
            m = rParser.match(line)
            if m:
                litres = int(m.groupdict().get('litros'))
                if litres == 0 or litres > MAX and last_value and abs(last_value - litres) > 150:
                    continue  # Wrong reading for sure
                else:
                    last_value = litres
                    store_reading(litres)
            os.system("touch %s/alive" % BASEDIR)
            logfile = open("/root/sensor.agua/agua.log", "aw")
            logfile.write(text)
            logfile.close()

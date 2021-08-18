#!/usr/bin/python

import os
import random
import serial
import time
import MySQLdb
import re
from datetime import date, datetime

BASEDIR="/root/sensor.agua"

rParser = re.compile(r"d: (?P<distancia>.*) h: (?P<altura>.*) v: (?P<litros>\d*)")

last_value = None

DB_HOST = '94.177.253.187'
DB_USER = 'client'
DB_PASS = 'los martes a las 7 al sol'
DB_NAME = 'deposito'

logfile = open("agua.log", "aw")
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.1)

def run_query(query='', commit=False):
    datos = [DB_HOST, DB_USER, DB_PASS, DB_NAME]

    conn = MySQLdb.connect(*datos) # Conectar a la base de datos
    cursor = conn.cursor()         # Crear un cursor
    cursor.execute(query)          # Ejecutar una consulta

    if commit:
        conn.commit()              # Hacer efectiva la escritura de datos
    cursor.close()                 # Cerrar el cursor
    conn.close()                   # Cerrar la conexion


# Esta es la consulta, insercion o actualizacion que vamos a lanzar
def send_to_db(litres):
   query = "INSERT INTO deposito (datetime, litros) VALUES (now(), %s)" % litres
   run_query(query, True)

os.system("rm -f %s/alive" % BASEDIR)

def store_reading(value):
    with open("%s/lecturas/%d" %(BASEDIR, time.time()), "w") as f:
        f.write(str(value))

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
           if last_value and abs(last_value - litres) > 150:
               continue # Wring reading for sure
           else:
              last_value = litres
              store_reading(litres)
       os.system("touch %s/alive" % BASEDIR)
       logfile = open("/root/sensor.agua/agua.log", "aw")
       logfile.write(line)
       logfile.close()

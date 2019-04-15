#!/usr/bin/python3

# WaterTower.py

# import pandas as pd
# import numpy as np
import RPi.GPIO as GPIO
# import subprocess
# import datetime
import time

# import os.path
# import logging
# import json
# from watertowerfunctions import write_file  #, get_weather, IFTTTmsg, check_web_response, weather_date_only    # read_scale, calculate,

# Turn on/off each valve based on CSV schedule
# Stop pump if Supply is empty
# Stop pump if Tower is full
# Turn on Pump if Tower Empty
# Check Tower every 2 seconds when valve is on
# If rain is predicted delay watering to see if rain falls (and how much)
# If rain is predicted and storage is full, consider emptying rain barrels depending on volume forecasted

# TODO:
# Add light flashing for problems
# need physical switch to turn off pump outide (maintenance mode)
# if something is wrong need better "pump stays off until problem fixed"
# add rain delay based on pressure sensor (water added) or weather info
# Create 2 sigma bars to determine when data is abnormal
# Leak detection by timing how long it takes before TowerFull shows empty after a refill
# Add console read for turning pump off for an amount of time for maintenance
# Add config file to read settings
# If TowerFullStatus is full, increase refresh rate
# Fix web panel, related to timing and delay function
# If cycling, write message to IF App.  If Error checks show problem, call IFAlert
# Use switch/case in GetTime function
# Test negative millis value else statement
# Allow status check during the delay (see Blick without delay example)
# remove datetime extra options, consider #ifdefine to keep code but remove extra during compile
# dont read temperature if empty
# Use Config type file to store pump run time. read from file during setup and store new value in each loop
# Consider instead of using timing to detect dry and cycle pumping, use flags

# TowerFullStatus       1=NotFull   0=Full       confirmed
# TowerEmptyStatus      1=Empty     0=NotEmpty   confirmed
# StorageFullStatus     1=Full      0=NotFull
# StorageEmptyStatus    1=Empty     0=NotEmpty   confirmed

# Pump_Runtime
# Tower_TimetoEmpty
# Pump_DryPumpTimeLimit
# Pump_Cycling

# # Pin Definitons:  (For pin "GPIO4" use "4")
Pin_Sensor_TowerFull = 17
Pin_Sensor_TowerEmpty = 27
Pin_Sensor_StorageFull = 4
Pin_Sensor_StorageEmpty = 14
Pin_Sensor_WaterFlow = 15
Pin_Sensors = [Pin_Sensor_TowerFull, Pin_Sensor_TowerEmpty, Pin_Sensor_StorageFull]#, Pin_Sensor_StorageEmpty]
Pin_Valve_Bed1Alley = 23
Pin_Valve_Bed2 = 24
Pin_Valve_Bed3 = 25
Pin_Valve_Bed4 = 8
Pin_Valve_Bed5andGarage = 7
Pin_Valve_Bed6Trees = 9
Pin_Valve_Bed7Perenial = 11
Pin_Valves = [Pin_Valve_Bed1Alley, Pin_Valve_Bed2, Pin_Valve_Bed3, Pin_Valve_Bed4, Pin_Valve_Bed5andGarage, Pin_Valve_Bed6Trees, Pin_Valve_Bed7Perenial]
Pin_LED_TowerFull = 2
Pin_LED_TowerEmpty = 3
Pin_LED_StorageFull = 22
Pin_LED_StorageEmpty = 10
Pin_LEDs = [Pin_LED_TowerFull, Pin_LED_TowerEmpty, Pin_LED_StorageFull, Pin_LED_StorageEmpty]
Pin_Pump = 18

# Pin Setup:
GPIO.setmode(GPIO.BCM)  # Broadcom/GPIO pin-numbering scheme not Board Pins
GPIO.setup(Pin_Sensors, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(Pin_Sensor_WaterFlow, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(Pin_Valves, GPIO.OUT)
GPIO.output(Pin_Valves, GPIO.LOW)
GPIO.setup(Pin_LEDs, GPIO.OUT)
GPIO.output(Pin_LEDs, GPIO.LOW)
GPIO.setup(Pin_Pump, GPIO.OUT)
GPIO.output(Pin_Pump, GPIO.LOW)
print("Setup Done")

# Initial state for Outputs:
# GPIO.output(Pin_Valves, GPIO.HIGH)        # Relay board in use uses HIGH as off
GPIO.output(Pin_Pump, GPIO.LOW)
time.sleep(5)
GPIO.output(Pin_Pump, GPIO.HIGH)
print("Set Hi")
try:
    time.sleep(5)
    GPIO.output(Pin_Pump, GPIO.LOW)
    print("Set Low")
    time.sleep(5)


except:
    # IFTTTmsg("WeightMonitor Exception")
    # logging.exception("WeightMonitor Exception")
    raise
    # # print("Exception")

# finally:
#     GPIO.cleanup()

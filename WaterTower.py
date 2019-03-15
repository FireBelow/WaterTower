#!/usr/bin/python3

# WaterTower.py

import pandas as pd
import numpy as np
import RPi.GPIO as GPIO
# import subprocess
import datetime
import time

# import os.path
import logging
import json
from watertowerfunctions import write_file, get_weather, IFTTTmsg, check_web_response, weather_date_only    # read_scale, calculate,

# Turn on/off each valve based on CSV schedule
# Stop pump if Supply is empty
# Stop pump if Tower is full
# Turn on Pump if Tower Empty
# Check Tower every 2 seconds when valve is on
# If rain is predicted delay watering to see if rain falls (and how much)
# If rain is predicted and storage is full, consider emptying rain barrels depending on volume forecasted

# Pin Definitons:  (For pin "GPIO4" use "4")
Pin_Sensor_TowerFull = X
Pin_Sensor_TowerEmpty = X
Pin_Sensor_StorageFull = X
Pin_Sensor_StorageEmpty = X
# Pin_Sensor_WaterFlow
Pin_Sensors = [Pin_Sensor_TowerFull, Pin_Sensor_TowerEmpty, Pin_Sensor_StorageFull, Pin_Sensor_StorageEmpty]
Pin_Valve_Bed1Alley = X
Pin_Valve_Bed2 = X
Pin_Valve_Bed3 = X
Pin_Valve_Bed4 = X
Pin_Valve_Bed5andGarage = X
Pin_Valve_Bed6Trees = X
Pin_Valve_Bed7Herbs = X
Pin_Valves = [Pin_Valve_Bed1Alley, Pin_Valve_Bed2, Pin_Valve_Bed3, Pin_Valve_Bed4, Pin_Valve_Bed5andGarage, Pin_Valve_Bed6Trees, Pin_Valve_Bed7Herbs]
Pin_Pump = X

# Pin Setup:
GPIO.setmode(GPIO.BCM) # Broadcom pin-numbering scheme
GPIO.setup(Pin_Sensors, GPIO.IN)
GPIO.setup(Pin_Valves, GPIO.OUT)
GPIO.setup(Pin_Pump, GPIO.OUT)

# Initial state for Outputs:
GPIO.output(Pin_Valves, GPIO.HIGH)        # Relay board in use uses HIGH as off

# try:
#     LogFileName = "/home/pi/Documents/Code/Log/Monitor.log"
#     logger = logging.getLogger("WeightMonitor")
#     logger.setLevel(logging.INFO)

#     # create the logging file handler
#     LogHandler = logging.FileHandler(LogFileName)
#     formatter = logging.Formatter('%(asctime)s-%(name)s-%(levelname)s-%(message)s')
#     LogHandler.setFormatter(formatter)

#     # # add handler to logger object
#     logger.addHandler(LogHandler)
#     logger.info("Program started")
    
#     MINUS_ONE_DAY = datetime.timedelta(days=1)
#     TODAY = datetime.datetime.now().date()
#     print(TODAY, type(TODAY))
#     YESTERDAY = TODAY - MINUS_ONE_DAY
#     # print(YESTERDAY, type(YESTERDAY))
#     YEAR = TODAY.year
#     # print(YEAR, type(YEAR))
#     ISO_DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S"
#     ISO_DATE_FORMAT = "%Y-%m-%d"
#     FILE_DATE_FORMAT = "%Y%m%d"

#     OUTPUTFILE = "/home/pi/Documents/Code/" + TODAY.strftime(FILE_DATE_FORMAT) + "_WeightLog.csv"
#     # print(OUTPUTFILE)
#     Notes = ""
#     jsonfilename = "/home/pi/Documents/Code/private.json"
#     with open(jsonfilename) as jsonfile:
#         jsondata = json.load(jsonfile)          # read json
#         # print(jsondata)
#         # json.dump(newdata, jsonfile)           #write json
#         Headers = jsondata['Headers']['All']
#         # print(Headers)
#     # Headers = "DateTime,WBigMed,WSmlMed,BigTemp,BigHum,SmlTemp,SmlHum,WMain,WDesc,WTemp,WPressure,WHumid,WWindSpd,WWindDir,WRain,WSnow,WVisible,WClouds,WSunrise,WSunset,Solar,UV,Precip1hr,PrecipToday,RawReadB,WBigStd,RawReadS,WSmlStd,Notes\n"
#     try:
#         open(OUTPUTFILE, 'r')
#         print("File already exists")
#     except:
#         # TODAY = time.strftime("%Y%m%d")
#         # OUTPUTFILE = "/home/pi/Documents/Code/" + str(TODAY) + "_WeightLog.csv"
#         print("File does not exist")
#         write_file(OUTPUTFILE, 'w', Headers)
#         calculate()

#     # WeatherData = subprocess.check_output(['/home/pi/Documents/Code/weather.sh'], universal_newlines=True)      #univ lines makes the output a text stream
#     # WeatherData = WeatherData.replace("\n","")
#     # print(WeatherData)

#     WeatherData = get_weather()
#     print(WeatherData)

#     def read_retry_multi(sensor, pin, retries=15, delay_seconds=2, platform=None, num_reads=5):
#         """Reads sensor using "read_retry" multiple times (num_reads) and returns
#         data as a list"""
#         HUMID_THRESHOLD = 300.0
#         TEMP_THRESHOLD = 110.0
#         data_list = []
#         while len(data_list) < num_reads:
#             humidity, temperature = Adafruit_DHT.read_retry(sensor, pin, retries, delay_seconds, platform)
#             if humidity is not None and temperature is not None:
#                 if temperature < TEMP_THRESHOLD and humidity < HUMID_THRESHOLD:
#                     # print(round(humidity,4), round(temperature, 4))
#                     TempHum = (round(humidity, 2), round(temperature, 2))
#                     data_list.append(TempHum)
#                     time.sleep(2)
#                 else:
#                     problem_msg = "TempHum threshold error, read again"
#                     print(problem_msg)
#                     logger.info(problem_msg)
#                     # IFTTTmsg(problem_msg)
#         return data_list

#     SENSOR_TYPE = Adafruit_DHT.DHT22
#     BIG_TEMPHUM_PIN = 16    # change to big Pin
#     DHT_NUM_READS = 5
#     DHT_STDEV_THRESHOLD = 20
#     DHT_THRESHOLD_TEMP_MAX = 1000
#     DHT_THRESHOLD_HUM_MAX = 101
#     RETRY_ATTEMPTS = 7       # number of times to reread sensor to get normal StDev
#     # BIGhumidity, BIGtemperature = Adafruit_DHT.read_retry(sensortype, BIG_TEMPHUM_PIN)       #def read_retry(sensor, pin, retries=15, delay_seconds=2, platform=None):
#     # print(Adafruit_DHT.read_retry(sensortype, BIG_TEMPHUM_PIN))
#     print("Read BIGtemphum")
#     BIGtemphum = read_retry_multi(SENSOR_TYPE, BIG_TEMPHUM_PIN, DHT_NUM_READS)        # def read_retry(sensor, pin, retries=15, delay_seconds=2, platform=None):
#     # print(BIGtemphum)
#     BIGtemperature = [temp * (9 / 5) + 32 for humid, temp in BIGtemphum]       # convert from Celcius to F
#     BIGhumidity = [humid for humid, temp in BIGtemphum]
#     print(round(np.std(BIGtemperature), 2), round(np.std(BIGhumidity), 2))
#     if np.std(BIGtemperature) > DHT_STDEV_THRESHOLD or np.std(BIGhumidity) > DHT_STDEV_THRESHOLD:
#         for i in range(2, RETRY_ATTEMPTS + 2):
#             print("TempHum StDev is too high (" + str(round(np.std(BIGtemperature), 2)) + ", " + str(round(np.std(BIGhumidity), 2)) + "), reading again...")
#             print(BIGtemphum)
#             BIGtemphum1 = BIGtemphum
#             time.sleep(3)
#             BIGtemphum = read_retry_multi(SENSOR_TYPE, BIG_TEMPHUM_PIN, DHT_NUM_READS)
#             if np.std(BIGtemperature) > DHT_STDEV_THRESHOLD or np.std(BIGhumidity) > DHT_STDEV_THRESHOLD:
#                 print("TempHum StDev is still too high")
#                 if np.std(BIGtemphum) < np.std(BIGtemphum1):
#                     BIGtemphum = BIGtemphum1
#                 else:
#                     BIGtemphum1 = BIGtemphum
#             else:
#                 print("Better reading recorded after " + str(i) + " tries")
#                 logger.info("BigDHT Retries: " + str(i))
#                 Notes = Notes + "BigDHT Retries: " + str(i)
#                 break
#         if i >= RETRY_ATTEMPTS:
#             logger.info(str(i) + " retries for BigDHT & stdevs(T&H): " + str(round(np.std(BIGtemperature), 2)) + " " + str(round(np.std(BIGhumidity), 2)))
#             logger.info(str(BIGtemphum))
#             IFTTTmsg(str(i) + " retries for BigDHT")
#     else:
#         print("StDev is below threshold of " + str(DHT_STDEV_THRESHOLD))
#     # print(BIGtemperature)
#     # print(BIGhumidity)
#     print(round(np.mean(BIGtemperature), 2),
#           round(np.median(BIGtemperature), 2),
#           round(np.std(BIGtemperature), 2))
#     print(round(np.mean(BIGhumidity), 2),
#           round(np.median(BIGhumidity), 2),
#           round(np.std(BIGhumidity), 2))

#     # Check for abnormal values
#     if np.median(BIGtemperature) > DHT_THRESHOLD_TEMP_MAX:
#         logger.info("Super high Big Temp: " + str(np.median(BIGtemperature)))
#         IFTTTmsg("Super high Big Temp")
#         BIGtemperature = [np.nan]
#     if np.median(BIGhumidity) > DHT_THRESHOLD_HUM_MAX:
#         logger.info("Super high Big Temp: " + str(np.median(BIGhumidity)))
#         IFTTTmsg("Super high Big Hum")
#         BIGhumidity = [np.nan]

#     SML_TEMPHUM_PIN = 13
#     print("Read SMLtemphum")
#     SMLtemphum = [(0, 0)]   # read_retry_multi(SENSOR_TYPE, SML_TEMPHUM_PIN, DHT_NUM_READS)        #def read_retry(sensor, pin, retries=15, delay_seconds=2, platform=None):
#     # print(SMLtemphum)
#     # print(SMLtemphum[0][0])
#     # print(SMLtemphum[:][0])
#     SMLtemperature = [temp * (9 / 5) + 32 for humid, temp in SMLtemphum]       # convert from Celcius to F
#     SMLhumidity = [humid for humid, temp in SMLtemphum]
#     # print(SMLtemperature)
#     # print(SMLhumidity)
#     print(round(np.mean(SMLtemperature), 2),
#           round(np.median(SMLtemperature), 2),
#           round(np.std(SMLtemperature), 2))
#     print(round(np.mean(SMLhumidity), 2),
#           round(np.median(SMLhumidity), 2),
#           round(np.std(SMLhumidity), 2))


# #    if BIGhumidity is not None and BIGtemperature is not None:
# #        print('Temp={0:0.1f}*F  Humidity={1:0.1f}%'.format(BIGtemperature, BIGhumidity))
# #    else:
# #        print('Failed to get reading. Try again!')

#     HX_NUM_READS = 15
#     HX_STDEV_THRESHOLD = 2000
#     BIG_DATA_PIN = 21
#     BIG_CLOCK_PIN = 20

#     BIGdata_raw = read_scale(READINGS=HX_NUM_READS, DATAPIN=BIG_DATA_PIN, CLOCKPIN=BIG_CLOCK_PIN)        # Read big scale
#     if np.std(BIGdata_raw) > HX_STDEV_THRESHOLD:
#         for i in range(2, RETRY_ATTEMPTS + 2):
#             print("Scale StDev is too high (" + str(round(np.std(BIGdata_raw), 2)) + "), reading again...")
#             BIGdata_raw1 = BIGdata_raw
#             time.sleep(2)
#             BIGdata_raw = read_scale(READINGS=HX_NUM_READS, DATAPIN=BIG_DATA_PIN, CLOCKPIN=BIG_CLOCK_PIN)        # Read big scale again
#             if np.std(BIGdata_raw) > HX_STDEV_THRESHOLD:
#                 print("Scale StDev is still too high")
#                 if np.std(BIGdata_raw) < np.std(BIGdata_raw1):
#                     BIGdata_raw1 = BIGdata_raw
#                 else:
#                     BIGdata_raw = BIGdata_raw1
#             else:
#                 print("Better reading recorded after " + str(i) + " tries")
#                 logger.info("BigHX Retries: " + str(i))
#                 Notes = Notes + "BigHX Retries: " + str(i)
#                 break
#         if i >= RETRY_ATTEMPTS + 2:
#             logger.info(str(i) + " retries for BigHX & stdev: "
#                         + str(round(np.std(BIGdata_raw), 2)))
#             IFTTTmsg(str(i) + " retries for BigHX")
#             BIGdata_raw = BIGdata_raw1
#     else:
#         print("StDev is below threshold of " + str(HX_STDEV_THRESHOLD))
#     print(round(np.median(BIGdata_raw), 2), round(np.std(BIGdata_raw), 2))
#     BIGdata = 0.0002311996 * np.median(BIGdata_raw) + 7.4413911706      # convert raw to meaningful
#     print(round(np.median(BIGdata), 2))
#     # BIGoffset = 0.0249121084 * np.median(BIGtemperature) - 2.4013925301        # temp calibration with slightly larger temp range but also more nosie
#     # BIGoffset = 0.0206347975 * np.median(BIGtemperature) - 2.0311528126     # temp calibration with smaller range but really good R^2
#     BIGoffset = 0.0371426355 * np.median(BIGhumidity) - 1.3708392923     # humid calibration with smaller range but really good R^2
#     # print(round(np.median(BIGoffset), 2))
#     BIGdata = BIGdata + BIGoffset
#     # print(round(np.median(BIGdata), 2))

#     SML_DATA_PIN = 26
#     SML_CLOCK_PIN = 19
#     # print("Read Small Scale")
#     SMLdata_raw = read_scale(READINGS=HX_NUM_READS, DATAPIN=SML_DATA_PIN, CLOCKPIN=SML_CLOCK_PIN)      # Read small scale
#     if np.std(SMLdata_raw) > HX_STDEV_THRESHOLD:
#         for i in range(2, RETRY_ATTEMPTS + 2):
#             print("Scale StDev is too high (" + str(round(np.std(SMLdata_raw), 2)) + "), reading again...")
#             SMLdata_raw1 = SMLdata_raw
#             time.sleep(2)
#             SMLdata_raw = read_scale(READINGS=HX_NUM_READS, DATAPIN=SML_DATA_PIN, CLOCKPIN=SML_CLOCK_PIN)        # Read small scale again
#             if np.std(SMLdata_raw) > HX_STDEV_THRESHOLD:
#                 print("Scale StDev is still too high")
#                 if np.std(SMLdata_raw) < np.std(SMLdata_raw1):
#                     SMLdata_raw1 = SMLdata_raw
#                 else:
#                     SMLdata_raw = SMLdata_raw1
#             else:
#                 print("Better reading recorded after " + str(i) + " tries")
#                 logger.info("Sml HX Retries: " + str(i))
#                 Notes = Notes + "SmlHX Retries: " + str(i)
#                 break
#         if i >= RETRY_ATTEMPTS:
#             logger.info(str(i) + " retries for SmlHX & stdev: " + str(round(np.std(SMLdata_raw), 2)))
#             IFTTTmsg(str(i) + " retries for SmlHX")
#             SMLdata_raw = SMLdata_raw1
#     else:
#         print("StDev is below threshold of " + str(HX_STDEV_THRESHOLD))
#     print(round(np.median(SMLdata_raw), 2), round(np.std(SMLdata_raw), 2))
#     SMLdata = 0.0001398789 * np.median(SMLdata_raw) + 208.9558945667      # convert raw to meaningful
#     print(round(np.median(SMLdata), 2))
#     # SMLoffset = -0.0033135985 * np.median(BIGhumidity) + 0.357458148      # temp calibration with fewer data points than big scale
#     SMLoffset = -0.0059644774 * np.median(BIGhumidity) + 0.2514229949      # humid calibration with fewer data points than big scale
#     # print(round(np.median(SMLoffset), 2))
#     SMLdata = SMLdata + SMLoffset
#     # print(round(np.median(SMLdata), 2))

#     BigMedian = round(np.median(BIGdata), 2)
#     SmlMedian = round(np.median(SMLdata), 2)
#     BigTempMedian = round(np.median(BIGtemperature), 2)
#     BigHumMedian = round(np.median(BIGhumidity), 2)
#     SmlTempMedian = round(np.median(SMLtemperature), 2)
#     SmlHumMedian = round(np.median(SMLhumidity), 2)
#     # Weather Data
#     BigRaw = np.median(BIGdata_raw)
#     BigRawStdev = round(np.std(BIGdata_raw), 1)
#     SmlRaw = np.median(SMLdata_raw)
#     SmlRawStdev = round(np.std(SMLdata_raw), 1)
#     # Notes

#     def find_outliers(DailyLogFile):
#         """This function determines if the current reading is
#         an outlier (i.e. outside Q1 and Q3 bounds)"""
#         low_datapoint_threshold = 20
#         with open(DailyLogFile, "r") as inputfile:
#             # print(inputfile)
#             filecontents = pd.read_csv(inputfile, delimiter=',', index_col="DateTime")
#         # print(filecontents.info())
#         n = filecontents["WBigMed"].count()
#         if n <= low_datapoint_threshold:
#             logger.info("Using yesterday log due to n(" + str(n) + ")<=" + str(low_datapoint_threshold))
#             DailyLogFile = "/home/pi/Documents/Code/" + YESTERDAY.strftime(FILE_DATE_FORMAT) + "_WeightLog.csv"
#             with open(DailyLogFile, "r") as inputfile:
#                 # print(inputfile)
#                 filecontents = pd.read_csv(inputfile, delimiter=',', index_col="DateTime")

#         Q1 = round(filecontents["WBigMed"].quantile(.25), 2)
#         Q3 = round(filecontents["WBigMed"].quantile(.75), 2)
#         low_outlier_threshold = Q1-(2*(Q3-Q1))
#         high_outlier_threshold = Q3+(2*(Q3-Q1))
#         # print(round(low_outlier_threshold, 2), round(high_outlier_threshold, 2))

#         return (round(low_outlier_threshold, 2), round(high_outlier_threshold, 2), n)

#     DAILY_LOG_TODAY = "/home/pi/Documents/Code/" + TODAY.strftime(FILE_DATE_FORMAT) + "_WeightLog.csv"
#     low_outlier_threshold, high_outlier_threshold, n = find_outliers(DAILY_LOG_TODAY)
#     print(low_outlier_threshold, high_outlier_threshold)
#     if BigMedian <= low_outlier_threshold:
#         problem_msg = "BigMedian Outlier Reading " + str(BigMedian) + "<" + str(low_outlier_threshold) + ", " + str(n)
#         logger.info(problem_msg)
#         IFTTTmsg(problem_msg)
#         # BigMedian = np.nan
#     elif BigMedian >= high_outlier_threshold:
#         problem_msg = "BigMedian Outlier Reading " + str(BigMedian) + ">" + str(high_outlier_threshold) + ", " + str(n)
#         logger.info(problem_msg)
#         IFTTTmsg(problem_msg)
#         # BigMedian = np.nan
#     else:
#         print("BigMedian Reading within IQR")

#     COMMA = ','
#     AllData = time.strftime(ISO_DATETIME_FORMAT) + COMMA        \
#         + str(BigMedian) + COMMA                                \
#         + str(SmlMedian) + COMMA                                \
#         + str(BigTempMedian) + COMMA                            \
#         + str(BigHumMedian) + COMMA                             \
#         + str(SmlTempMedian) + COMMA                            \
#         + str(SmlHumMedian) + COMMA                             \
#         + str(WeatherData) + COMMA                              \
#         + str(BigRaw) + COMMA                                   \
#         + str(BigRawStdev) + COMMA                              \
#         + str(SmlRaw) + COMMA                                   \
#         + str(SmlRawStdev) + COMMA                              \
#         + Notes + "\n"

#     # write_file(OUTPUTFILE, 'a', Headers)       #to update headers in the middle of the file
#     write_file(OUTPUTFILE, 'a', AllData)
#     # IFTTTmsg("WeightMonitor Success!")

#     # calculate()     # use if need to force a calculate

except:
    # IFTTTmsg("WeightMonitor Exception")
    # logging.exception("WeightMonitor Exception")
    # raise
    # # print("Exception")

finally:
    GPIO.cleanup()

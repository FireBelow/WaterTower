#!/usr/bin/c

#include <Bridge.h>
//#include <Console.h>
#include <FileIO.h>
#include <OneWire.h>
//#include <EEPROM.h>
//#include <EEPROMWriteAnything.h>
//#include <HttpClient.h>
//#include <Mailbox.h>
#include <Process.h>
//#include <YunClient.h>
//#include <YunServer.h>
//#include <PlotlyYun.h>
//#include <YunMessenger.h>
//#include <Debug.h>
#include <MemoryFree.h>

/*
Water Tower Control
This program controls the water pump needed to pump water up to the garden water tower to increase the pressure for the irrigation system.

D0-D13 Digital Pins (0=open, 1=closed, Max current provided: 40 mA)
A0-A5 Analog Pins (Measures GND to 5V, 1024 different values)

--Sub Function List:
StopPump()
StartPump(StorageEmptyStatus, TowerFullStatus, TowerEmptyStatus)
SetLed(LED Name, State)
GetTheDateTime(FormatType)
SaveStringtoSD(StringtoSave)
CheckForClient(YunClient)
FlashLightDelay(Delay)
availableMemory()

TowerTemp Sensor Address 28,D3,B7,5E,6,0,0,81
StorageTemp Sensor Address 28,CF,B7,5E,6,0,0,A7

TODO:
Add light flashing for problems
need physical switch to turn off pump outide (maintenance mode)
if something is wrong need better "pump stays off until problem fixed"
add rain delay based on pressure sensor (water added) or weather info
Create 2 sigma bars to determine when data is abnormal
Leak detection by timing how long it takes before TowerFull shows empty after a refill
Add console read for turning pump off for an amount of time for maintenance
Add config file to read settings
If TowerFullStatus is full, increase refresh rate
Fix web panel, related to timing and delay function
If cycling, write message to IF App.  If Error checks show problem, call IFAlert
Use switch/case in GetTime function
Test negative millis value else statement
Allow status check during the delay (see Blick without delay example)
remove datetime extra options, consider #ifdefine to keep code but remove extra during compile
dont read temperature if empty
Use Config type file to store pump run time. read from file during setup and store new value in each loop
Consider instead of using timing to detect dry and cycle pumping, use flags

TowerFullStatus       1=NotFull   0=Full       confirmed
TowerEmptyStatus      1=Empty     0=NotEmpty   confirmed
StorageFullStatus     1=Full      0=NotFull
StorageEmptyStatus    1=Empty     0=NotEmpty   confirmed
*/

//debug toggle, comment out to turn off debugging mode
//#define DEBUG_CONSOLE     //debugging info sent to console
#define DEBUG_LOG         //debugging info sent to log
//declare debug commands/functions
#define PROBLEM 1
#define OK 0
#define TRUE 1
//#define MAINTENANCE-MODE    //in this mode the StartPump function just exits

#ifdef DEBUG_CONSOLE
  #define DEBUG_PRINT(x) Console.print(x)
  //#define DEBUG_PRINTDEC(x) Console.print(x, DEC)
  #define DEBUG_PRINTLN(x) Console.println(x)
#else
  #define DEBUG_PRINT(x)
  //#define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
#endif

const byte BufferConsole = 8;         // Console Buffer Size max=255
const byte DefaultRate = 120;    //Max is 255. Normal RefreshRate for normal operation (aka not actively pumping) (in seconds)
const byte FastRefreshRate = 1;   //Faster RefreshRate to better determine when to turn off the pump (in seconds)
byte RefreshRate = DefaultRate;      //Time between refreshing the sensor readings (in seconds)

//Long can be: -2,147,483,648 to 2,147,483,647        UnsignedLong can be: 0 to 4,294,967,295
const long DryPumpingMaxTime = 60L * (1000L); //Max time in miliseconds that the pump will run without raising the towerempty switch before turning off the pump
const long DryPumpWaitTime = 240L * (60L * 1000L); //How many minutes to wait after pump ran dry for too long
long CyclingDelay = 0;  //default to NO cycling delay
boolean TowerErrorFlag = OK;     //used to stop operations to prevent damage or water overflowing
boolean StorageErrorFlag = OK;   //used to prevent pump damage

float TemperatureTower = 0.0;
float TemperatureStorage = 0.0;

unsigned long LoopStartTime = 0;
unsigned long PumpRunTimeStart = 0;
unsigned long PumpRunTimeCurrent = 0;
unsigned long PumpRunTimeTotal = 0;
unsigned long CurrentTime = 0;
unsigned long TowerTimetoEmpty = 0;
unsigned long TowerTimetoEmptyStart = 0;
unsigned long PumpOffTime = 0;

String DateTimeString = "";              //For storing the Date/Time retrieved from Linux process
//String FunctionResultString = "";             // declaration and initialization
String LogDataToSave = "";          //consider using the reserve() buffer function
const String Comma = ",";
String Today = "";
//String Notes = "";      //change to char
const String FilePath = "/mnt/sd/20180705_WaterTowerDatalog.csv";                //filepath for sd card saving
byte n = 1;                             //place holder, so datetime runs only once at startup

//plotly graph1("ccpresa4ae");//, "America/Montreal");
//plotly graph2("p4a1zvtyfa");//, "America/Montreal");
//plotly graph3("dpufi5ajzf");//, "America/Montreal");
//plotly graph4("m1cb8pyj61");//, "America/Montreal");
//plotly graph5("zkvbm3ycoa");//, "America/Montreal");
//plotly graph6("3g4rhyr4dc");//, "America/Montreal");
//plotly graph7("7f0enkt6oi");//, "America/Montreal");

// The list of Digital Pin Assignments:
//byte SerialRx = 0;             // Reserved for Serial Rx
//byte SerialTx = 1;             // Reserved for Serial Tx
const byte TowerFullPin = 3;        // switched due to wiring mistake on the arduino side
const byte TowerEmptyPin = 2;       //
const byte StorageFullPin = 5;      // switched due to wiring mistake on the arduino side
const byte StorageEmptyPin = 4;     //
const byte PumpRelayPin = 6;              //
//const byte Reserved = 7;             //
const byte TowerTempPin = 8;             //
const byte StorageTempPin = 9;             //
const byte TowerFullLEDPin = 10;          //
const byte TowerEmptyLEDPin = 11; //
const byte StorageFullLEDPin = 12;                //
const byte StorageEmptyLEDPin = 13;       //also flashes each second the sketch is running

boolean TowerFullStatus;
boolean TowerEmptyStatus;
boolean StorageFullStatus;
boolean StorageEmptyStatus;
boolean TowerTempStatus;
boolean StorageTempStatus;

boolean PriorTowerFullStatus;        //To see when the status changes
boolean PriorTowerEmptyStatus;
boolean PriorStorageFullStatus;
boolean PriorStorageEmptyStatus;

// The list of Analog Pin Assignments:
//const int StoragePressure = A0;       // A pressure sensor used to guess how much water is in storage
//const int PumpVibe = A1;              // A vibration sensor used to determine if the pump is on or off
//const int BattVoltage = A2;          // A temperature sensor used to monitor the Tower water temperature
//const int Reserved = A3;           // A temperature sensor used to monitor the Storage water temperature
//const int Reserved = A4;           // A simple voltage divider to read the battery voltage

//Create a YunServer instance called "WaterServer" to allow communication and REST commands
//YunServer WaterServer;

// the setup routine runs once when you press reset or restart the device
void setup() {
  delay(60000);                   //delay to avoid stopping linux side during reboot of linux side (when sketch keeps running)
  // Set the pins as INPUT or OUTPUT:
  pinMode(TowerFullPin, INPUT);
  pinMode(TowerEmptyPin, INPUT);
  pinMode(StorageFullPin, INPUT);
  pinMode(StorageEmptyPin, INPUT);
  pinMode(PumpRelayPin, OUTPUT);
  pinMode(TowerTempPin, INPUT);
  pinMode(StorageTempPin, INPUT);
  pinMode(TowerFullLEDPin, OUTPUT);
  pinMode(TowerEmptyLEDPin, OUTPUT);
  pinMode(StorageFullLEDPin, OUTPUT);
  pinMode(StorageEmptyLEDPin, OUTPUT);
  // All Analog Pins are INPUTs by default

  //Default Values for Initial startup
  digitalWrite(PumpRelayPin, LOW);
  digitalWrite(TowerFullLEDPin, LOW);
  digitalWrite(TowerEmptyLEDPin, LOW);
  digitalWrite(StorageFullLEDPin, LOW);
  digitalWrite(StorageEmptyLEDPin, LOW);

  //start console
  digitalWrite(StorageFullLEDPin, HIGH);   //makes startup sequence visible
  Bridge.begin();                         //Bridge before "console"
  // Wait for Console port to connect, don't do anything unless console connects
  //delay(10000);
  #ifdef DEBUG_CONSOLE
    Console.begin();
    while (!Console);
  #endif
  if (Console) {
    Console.flush();
    Console.buffer(BufferConsole);
    DEBUG_PRINTLN("Console Started");
  }

  //WaterServer.listenOnLocalhost();        // (no one from the external network could connect)
  //WaterServer.noListenOnLocalhost();        //I believe this allows outside connections (not just from within the Yun, "local")
  //Start Server
  //WaterServer.begin();

  FileSystem.begin();               //For SD card saving

//  Process plotlyscript;
//  plotlyscript.runShellCommand("/mnt/sd/run_plotly.sh");
//  while (plotlyscript.running());

//  graph1.timezone = "America/Montreal";
//  graph2.timezone = "America/Montreal";
//  graph3.timezone = "America/Montreal";
//  graph4.timezone = "America/Montreal";
//  graph5.timezone = "America/Montreal";
//  graph6.timezone = "America/Montreal";
//  graph7.timezone = "America/Montreal";

  //graph1.world_readable = "false";
  //graph2.world_readable = "false";
  //graph3.world_readable = "false";

  //graph1.world_readable = "false";
  //graph2.world_readable = "false";
  //graph3.world_readable = "false";
  
  //graph1.convertTimestamp = false;
  //graph2.convertTimestamp = false;
  //graph3.convertTimestamp = false;

  if (!FileSystem.exists(FilePath.c_str())) {
    DEBUG_PRINTLN("File does not exist");
    LogDataToSave = "DateTime,PumpOn,TowerF,TowerE,TempT,StorageF,StorageE,TempS,PumpRunTimeTotal,Notes";   //save column titles as headers if this is the first time the file will be created (ie doesn't exist...yet)
#ifdef DEBUG_LOG    //if in debug mode, all debug title to column header
    LogDataToSave = LogDataToSave + ",avMem,FreMem,PumpStart,PumpCurrent,TowerTimetoEmpty,PumpOffTime,CycleDelay";
#endif
    SaveStringtoSD(LogDataToSave);
    //DEBUG_PRINTLN.println(FunctionResultString);
    DEBUG_PRINTLN(LogDataToSave);
  }
  else {
    //DEBUG_PRINTLN(F("File exists"));
  }
  DateTimeString = RunScript(3,"");
  LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "RESET";
  //FunctionResultString =
  SaveStringtoSD(LogDataToSave);
  //DEBUG_PRINTLN.println(FunctionResultString);
  DEBUG_PRINTLN(LogDataToSave);

  Today = DateTimeString.substring(6,8);        //save days to tell if the day changed

  TemperatureTower = GetTemp(TowerTempPin);
  TemperatureStorage = GetTemp(StorageTempPin);

  digitalWrite(StorageFullLEDPin, LOW);         //turn off light after setup
  DEBUG_PRINTLN("Setup Done");
}

// the loop routine runs over and over again forever:
void loop() {
  if (n==1){       //only run once at startup
    RunScript(2,"Im awake (WaterTower Reset)");
    //DateTimeString = GetTheDateTime(Format);
    //DateTimeString = RunScript(3,"");
    //LogDataToSave1 = DateTimeString + LogDataToSave1;
    //SaveStringtoSD(LogDataToSave1,"","");
    //DEBUG_PRINTLN(LogDataToSave1);
    n=0;      //keep this loop from running again
  }
  
  LoopStartTime = millis ();          //must use unsigned long
  //DEBUG_PRINTLN("Start Loop - ");
  //DEBUG_PRINTLN(millis());

  // Get clients coming from YunServer
  //YunClient client = WaterServer.accept();

  PriorTowerFullStatus = TowerFullStatus;        //To see when the status changes
  PriorTowerEmptyStatus = TowerEmptyStatus;
  PriorStorageFullStatus = StorageFullStatus;
  PriorStorageEmptyStatus = StorageEmptyStatus;

  TowerFullStatus = digitalRead(TowerFullPin);
  TowerEmptyStatus = digitalRead(TowerEmptyPin);
  StorageFullStatus = digitalRead(StorageFullPin);
  StorageEmptyStatus = digitalRead(StorageEmptyPin);
  TowerTempStatus = digitalRead(TowerTempPin);
  StorageTempStatus = digitalRead(StorageTempPin);

  // Read the state of each Analog INPUT pin:
  //  int StoragePressureStatus = analogRead(StoragePressure);
  //  int PumpVibeStatus = analogRead(PumpVibe);
  //  int BattVoltageStatus = analogRead(BattVoltage);
  /*  Console.println(StoragePressureStatus);      //Commented out until sensors are connected
    Console.println(PumpVibeStatus);
    Console.println(BattVoltageStatus);
  */

  // Error Condition Checking
  DEBUG_PRINT(F("Error Check..."));
  //Tower Full and Empty
  if (TowerFullStatus == OK && TowerEmptyStatus == PROBLEM) {    //TowerFull shows Full and Tower Empty shows Empty...ERROR. Full sensor probably stuck
    DateTimeString = RunScript(3,"");
    LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "TOWER ERROR";
    SaveStringtoSD(LogDataToSave);
    DEBUG_PRINTLN(F("***ERROR: Tower Full and Empty"));
    RunScript(2,"***ERROR: Tower Full and Empty");
    TowerErrorFlag = PROBLEM;
    //FlashTowerLights()
  }
  else {
    //StopFlashTowerLights()
    if (TowerErrorFlag == PROBLEM) {
      RunScript(2,"Tower Error Cleared");
      DateTimeString = RunScript(3,"");
      LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "Tower Error Cleared";
      SaveStringtoSD(LogDataToSave);
      TowerErrorFlag = OK;
    }
  }
  //Storage Full and Empty
  if (StorageFullStatus == TRUE && StorageEmptyStatus == PROBLEM) {     //different logic for ones and zeros
    DateTimeString = RunScript(3,"");
    LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "STORAGE ERROR";
    SaveStringtoSD(LogDataToSave);
    DEBUG_PRINTLN(F("***ERROR: Storage Full and Empty"));
    RunScript(2,"***ERROR: Storage Full and Empty");
    StorageErrorFlag = PROBLEM;
    //FlashStorageLights()
  }
  else {
    //StopFlashStorageLights()
    if (StorageErrorFlag == PROBLEM) {
      RunScript(2,"Storage Error Cleared");
      DateTimeString = RunScript(3,"");
      LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "Storage Error Cleared";
      SaveStringtoSD(LogDataToSave);
      StorageErrorFlag = OK;
    }
  }
  DEBUG_PRINTLN(F("Done"));

  // If Statements to determine what the water status is

  //DateTimeString = RunScript(3,"");                      //Get the current time
  //Console.println(DateTimeString);

  DEBUG_PRINT(TowerFullStatus);
  DEBUG_PRINT(" ");
  //If TowerFull is true-StopPump, else StartPump
  if (TowerFullStatus == OK) {             //If Tower full
    DEBUG_PRINTLN(F("Tower Full"));
    digitalWrite(TowerFullLEDPin, HIGH);  //Turn on LED
    StopPump();                           //Stop Pump
  }
  else {                                  //Tower Not Full
    DEBUG_PRINTLN(F("Tower Not Full"));
    if (TowerEmptyStatus == PROBLEM) {          //Tower Empty
      DEBUG_PRINTLN("Tower Empty");
      digitalWrite(TowerFullLEDPin, LOW);  //Turn off LED
      StartPump();
    }
  }

  DEBUG_PRINT(TowerEmptyStatus);
  DEBUG_PRINT(F(" "));
  //If TowerEmpty is true-StartPump, else nothing
  if (TowerEmptyStatus == PROBLEM) {    //Tower Empty
    if (TowerFullStatus == PROBLEM) {     //and Tower not full (just error check)
      DEBUG_PRINTLN(F("Tower Empty"));
      digitalWrite(TowerEmptyLEDPin, LOW);  //Turn off LED
      StartPump();
    }
  }
  else {                                //Tower Not Empty, has some water
    DEBUG_PRINTLN(F("Tower Not Empty"));
    digitalWrite(TowerEmptyLEDPin, HIGH);  //Turn on LED
  }

  //If both TowerFull and TowerEmpty are false, Tower has some water
  //if (TowerFullStatus == PROBLEM && TowerEmptyStatus == OK) {
    //DEBUG_PRINTLN(F("Tower Partly Full"));
  //}

  DEBUG_PRINT(StorageFullStatus);
  DEBUG_PRINT(F(" "));
  //If StorageFull is true-light LED
  if (StorageFullStatus == TRUE) {         //Storage Full
    if (StorageEmptyStatus == OK) {      //just error checking
      DEBUG_PRINTLN(F("Storage Full"));
        RunScript(2,"Storage Full");
      digitalWrite(StorageFullLEDPin, HIGH);  //Turn on LED
    }
  }
  else {                                  //Storage Not Full
    DEBUG_PRINTLN(F("Storage Not Full"));
    digitalWrite(StorageFullLEDPin, LOW);  //Turn off LED
  }

  DEBUG_PRINT(StorageEmptyStatus);
  DEBUG_PRINT(F(" "));
  //If StorageEmpty is true-StopPump
  if (StorageEmptyStatus == PROBLEM) {
    DEBUG_PRINTLN(F("Storage Empty"));
    StopPump();
    digitalWrite(StorageEmptyLEDPin, LOW);  //Turn off LED
    if (PriorStorageEmptyStatus == OK) {    //If there is a change in the readings the storage was full and is now empty, send message
      DateTimeString = RunScript(3,"");
      LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "NO WATER";
      SaveStringtoSD(LogDataToSave);
      RunScript(2,"NO WATER");
    }
  }
  else {
    DEBUG_PRINTLN(F("Storage Not Empty"));
    digitalWrite(StorageEmptyLEDPin, HIGH);  //Turn on LED
    if (PriorStorageEmptyStatus == PROBLEM) {
       RunScript(2,"Water Storage Added");
    }
  }

  //If both storageFull and StorageEmpty are false, Storage has some water
  //if (StorageFullStatus == OK && StorageEmptyStatus == OK) {
    //DEBUG_PRINTLN(F("Storage Partly Full"));
  //}

  //DEBUG_PRINTLN(digitalRead(TowerFullLEDPin));
  //DEBUG_PRINTLN(digitalRead(TowerEmptyLEDPin));
  //DEBUG_PRINTLN(digitalRead(StorageFullLEDPin));
  //DEBUG_PRINTLN(digitalRead(StorageEmptyLEDPin));

  //CheckConsoleText();       //check for notes to add to the log

  DateTimeString = RunScript(3,"");
  //DEBUG_PRINTLN("Time");
  //Console.println(freeMemory());
  //availableMemory();
  //Console.println(F("DateTime,TFull,TEmpty,TempTower,SFull,SEmpty,TempStorage,PumpRunTimeTotal"));
  LogDataToSave = DateTimeString + Comma + digitalRead(PumpRelayPin) + Comma + TowerFullStatus + Comma + TowerEmptyStatus + Comma + TemperatureTower;
  LogDataToSave = LogDataToSave + Comma + StorageFullStatus + Comma + StorageEmptyStatus + Comma + TemperatureStorage + Comma + PumpRunTimeTotal + Comma;  //ends with comma for notes
#ifdef DEBUG_LOG      //if in Debug mode, also print available memory and free memory and pump timings
  LogDataToSave = LogDataToSave + Comma + availableMemory() + Comma + freeMemory() + Comma + PumpRunTimeStart + Comma + PumpRunTimeCurrent + Comma + TowerTimetoEmpty + Comma + PumpOffTime + Comma + CyclingDelay;
#endif
  SaveStringtoSD(LogDataToSave);               //Send current data and time to SD card for logging
  //Console.print(FunctionResultString);                    //saved or not
  DEBUG_PRINTLN(LogDataToSave);                           //what was saved

  //Send the data to Plot.ly (use float, char, or string ?)
  //graph1.plot(DateTimeString, digitalRead(PumpRelayPin));   //most accurate loop time if graph1 is at the end
  //graph2.plot(DateTimeString, TowerFullStatus);
  //graph3.plot(DateTimeString, TowerEmptyStatus);
  //graph4.plot(StorageFullStatus);
  //graph5.plot(StorageEmptyStatus);

  //Console.println(DateTimeString.indexOf('7'));       //find a character
  //Console.println(DateTimeString.charAt(6));          //read character at a character position
  //Console.println(DateTimeString.substring(6,8));       //debug
  //Today = DateTimeString.substring(6,8);               //saves day to string
  //Console.println(Today);
  if (Today=DateTimeString.substring(6,8)) {            //****************add bee version
    DEBUG_PRINTLN("No Day Change");
  }
  else {
    DEBUG_PRINTLN("Day Changed");
    RunScript(2,"New Day");
    Today = DateTimeString.substring(6,8);
  }

  //CheckForClient(client);

  RunScript(4,"");          //Run script to run python program to plot data in Plotly
  
  //Console.flush();
  DEBUG_PRINT(F("Loop Done - "));
  DEBUG_PRINTLN(millis() - LoopStartTime);
  FlashLightDelay((RefreshRate * 1000UL) + CyclingDelay);        // X millisecond delay in between reads, "L" makes 1000 a long instead of int, plus delay if cycling is occurring

  // Close server connection and free resources.
  //client.stop();
  //DEBUG_PRINTLN("Loop End");
}

void StartPump() {
  //This function will check for water and then turn ON the Water Pump
  //Check for Pump Status
  #ifdef MAINTENANCE-MODE
    return;
  #endif
  if (TowerErrorFlag == PROBLEM || StorageErrorFlag == PROBLEM) {   //either error detected
    DEBUG_PRINTLN(F("***ERROR, Pump Disabled"));
    if (TowerErrorFlag == PROBLEM) RunScript(2,"TowerErrorFlag,PUMP Disable");
    if (StorageErrorFlag == PROBLEM) RunScript(2,"StorageErrorFlag,PUMP Disable");
    DateTimeString = RunScript(3,"");
    LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "ERROR FLAG,PUMP Disable";
    SaveStringtoSD(LogDataToSave);
    return;
  }
  else {    //no errors
    DEBUG_PRINTLN(F("No Errors, Pump Enabled"));
    if (digitalRead(PumpRelayPin) == HIGH) {        //if pump already on
      DEBUG_PRINTLN(F("->Pump Already On"));
      PumpRunTimeCurrent = millis() - PumpRunTimeStart;       //How long has the pump been on
      TowerTimetoEmpty = 0;
      DEBUG_PRINTLN(PumpRunTimeCurrent);

      //If Pump ran too long without raising the towerempty switch, turn off the pump
      if (PumpRunTimeCurrent > DryPumpingMaxTime && TowerEmptyStatus == PROBLEM) {
        StopPump();           //turn off pump
        RunScript(2,"Dry Pumping");
        DEBUG_PRINTLN(F("Dry Pumping"));    //can't combine these rows into one display line, different data types
        DEBUG_PRINT(PumpRunTimeCurrent);
        DEBUG_PRINT(">");
        DEBUG_PRINTLN(DryPumpingMaxTime);
        DateTimeString = RunScript(3,"");
        LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "DRY PUMP";
        SaveStringtoSD(LogDataToSave);
        delay(DryPumpWaitTime);
      }
    }
    else {
      DEBUG_PRINTLN(F("->Pump Not Already On"));
      // Check that Tower is empty and Storage is not empty
      if (StorageEmptyStatus == OK && TowerFullStatus == PROBLEM) {
        DEBUG_PRINTLN(F("->Water Needed and Available"));
        DEBUG_PRINT(F("->Starting Pump..."));
          digitalWrite(PumpRelayPin, HIGH);         //turn on pump
        if (PriorStorageEmptyStatus == PROBLEM){
          delay(60000);                   //if the storage was previously empty (it might still be refilling), give it extra time (1 min) to refill before regularly refreshing
        }
        PumpRunTimeStart = millis();            //record pump start time
        //Increase the RefreshRate while the pump is on
        RefreshRate = FastRefreshRate;
        DEBUG_PRINTLN(F("Pump Started"));
      }
      else {
        DEBUG_PRINTLN(F("->No Water"));
      }
    }
  }
}

void StopPump() {
  //This function will turn OFF the Water Pump
  const unsigned long CyclingWaitTime = 4 * (60 * 60 * 1000UL); //How many hours to wait if the pump starts to cycle on and off frequently
  const unsigned long CyclingMinTime = 5 * (1000UL); //Min run time (seconds) that indicated cycling occurred, pump really only turns on if the tower is empty

  DEBUG_PRINT(F("->Stop Pump..."));
  //Check if pump is already off
  if (digitalRead(PumpRelayPin) == LOW) {      //pump already off
    DEBUG_PRINTLN(F("Pump Already Off"));
    TowerTimetoEmpty = millis() - TowerTimetoEmptyStart;
    //float TowerTemperature = GetTemp(TowerTempPin);
    //float StorageTemperature = GetTemp(StorageTempPin);
  }
  else {                                           //pump is running
    DEBUG_PRINTLN(F("Pump Not Already Off"));
    DEBUG_PRINT(F("->Stopping Pump..."));
    digitalWrite(PumpRelayPin, LOW);                //Turn off Pump
    RefreshRate = DefaultRate;                     //Reduce RefreshRate back to normal
    //Add a check for Vibe sensor to make sure pump is off
    DEBUG_PRINTLN(F("Pump Off"));

    //Record total Pump Run Time (cummulative)
    CurrentTime = millis();
    TowerTimetoEmptyStart = millis();
    //if (CurrentTime > 0) {

    //}
    PumpRunTimeCurrent = CurrentTime - PumpRunTimeStart;
    //If current pump time is negative, skip adding this round
    if (PumpRunTimeCurrent > 0) {
      PumpRunTimeTotal = PumpRunTimeTotal + PumpRunTimeCurrent;
    }
    else {       //add the negative part (subtraction)
      //PumpRunTimeTotal = PumpRunTimeTotal - (PumpRunTimeCurrent) + (2147483647 - PumpRunTimeStart);
    }

    //Prevent cycling when low water
    if (CyclingDelay == CyclingWaitTime) {        //reset pump cycling
      CyclingDelay = 0;  //reset cycling delay
    }
    if (PumpRunTimeCurrent < CyclingMinTime) {   //pump is cycling on and off repeatedly if the pump only ran for a short time
      CyclingDelay = CyclingWaitTime;
      DateTimeString = RunScript(3,"");
      LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "CYCLING";
      SaveStringtoSD(LogDataToSave);
      DEBUG_PRINT(PumpRunTimeCurrent);    //can't combine these rows into one display line, different data types
      DEBUG_PRINT(">");
      DEBUG_PRINTLN(CyclingMinTime);
      DEBUG_PRINT(F("Pump Cycling ("));   //can't combine these rows into one display line, different data types
      DEBUG_PRINT(CyclingWaitTime + Comma + CyclingDelay + Comma);
      DEBUG_PRINT((RefreshRate * 1000L) + CyclingDelay);
      DEBUG_PRINTLN(F(")"));
      RunScript(2,"Cycling");
    }
    else {
      DEBUG_PRINTLN(F("No Cycling"));     //**************add cycling cleared if statement
    }

    DEBUG_PRINT(F("Pump Run Time: "));
    DEBUG_PRINTLN(PumpRunTimeTotal);
    PumpRunTimeStart = 0;
    PumpRunTimeCurrent = 0;

    //Get Temp of Tower and Storage
    DEBUG_PRINT(F("Tower: "));
    TemperatureTower = GetTemp(TowerTempPin);
    if (TemperatureTower == -2000) TemperatureTower = GetTemp(TowerTempPin);
    DEBUG_PRINT(TemperatureTower);
    DEBUG_PRINTLN(F(" DegF"));
    DEBUG_PRINT(F("Storage: "));
    TemperatureStorage = GetTemp(StorageTempPin);
    if (TemperatureStorage == -2000) TemperatureStorage = GetTemp(StorageTempPin);
    DEBUG_PRINT(TemperatureStorage);
    DEBUG_PRINTLN(F(" DegF"));

    //Send Temperatures to Plotly
    //graph6.plot(TemperatureTower);
    //graph7.plot(TemperatureStorage);
  }
}

float GetTemp(byte SubTempPin) {
  //This function returns the temperature from one DS18B20 in DEG F
  //DEBUG_PRINTLN(F("Get Temp"));
  //Set OneWire Pin
  OneWire Bus(SubTempPin);
  byte data[12];
  byte addr[8];
  if ( !Bus.search(addr)) {           //no sensors on chain, reset search
    Bus.reset_search();
    DEBUG_PRINT(F("-1000 on:"));
    DEBUG_PRINTLN(SubTempPin);
    DateTimeString = RunScript(3,"");
    LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "NO TEMP:" + SubTempPin;   //include pin# with error
    SaveStringtoSD(LogDataToSave);
    return -1000;
  }
  //Check if data is valid via CRC
  if ( OneWire::crc8( addr, 7) != addr[7]) {            //CRC is not valid!
    DEBUG_PRINTLN(F("CRC error,-2000, on:"));
    DEBUG_PRINTLN(SubTempPin);
    DateTimeString = RunScript(3,"");
    LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + "CRC ERROR:" + SubTempPin;   //include pin# with error
    SaveStringtoSD(LogDataToSave);
    return -2000;
  }
  /*
    //Check device type
    if ( addr[0] == 0x10) {
      Console.print("Device is a DS18S20 family device.\n");
    }
    else if ( addr[0] == 0x22) {
      Console.print("Device is a DS1820 family device.\n");
    }
    else if ( addr[0] == 0x28) {
      Console.print("Device is a DS18B20 family device.\n");
    }
    else {
      Console.print("Device family is not recognized: 0x");
      Console.println(addr[0],HEX);
      //return -3000;
    }
  */
  //Used to display the device address
  //for (int j = 0; j < 9; j++) { // we need 9 bytes
  //  Console.print(addr[j], HEX);
  //  Console.print(",");
  //}
  //Console.println();

  Bus.reset();
  Bus.select(addr);
  Bus.write(0x44, 1); // start conversion, with parasitic power at the end
  delay(750);             //wait for conversion

  byte presence = Bus.reset();
  //Console.println(presence);
  Bus.select(addr);
  Bus.write(0xBE); // Read Scratchpad
  //Read scratchpad data (temp)
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = Bus.read();
  }
  //Print Scratchpad data (temp data)
  //for (int i = 0; i < 9; i++) { // we need 9 bytes
  //  Console.println(data[i], HEX);
  //}
  Bus.reset_search();
  byte LSB = data[0];
  byte MSB = data[1];
  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = ((tempRead / 16) * 1.8) + 32;
  return TemperatureSum;
}

// A Function to get the date and/or time
/*String GetTheDateTime(String SubFormatType) {
  //RunScript();        //old check for network connection
  // get the time that this sketch started:
  String SubDateTimeString;
  //SubFormatType = String(SubFormatType);
  Process LinuxTime;
  //DEBUG_PRINTLN(SubFormatType);
  if (SubFormatType == "Default") {           //use switch instead
    LinuxTime.runShellCommand("date");
  }
  if (SubFormatType == "Date") {
    LinuxTime.runShellCommand("date +%Y%m%d");            //old: +%Y-%m-%d
  }
  if (SubFormatType == "Time") {
    LinuxTime.runShellCommand("date +%H%M%S");            //old: +%H:%M.%S
  }
  if (SubFormatType == "DateTime") {
    LinuxTime.runShellCommand("date +%Y%m%d%H%M%S");       //old: +%Y-%m-%d:%H:%M.%S
  }

  SubDateTimeString = "";
  while (LinuxTime.available()) {
    char c = LinuxTime.read();
    SubDateTimeString += c;
  }

  SubDateTimeString.trim();                       //removes: single space (ASCII 32), tab (ASCII 9), vertical tab (ASCII 11), form feed (ASCII 12), carriage return (ASCII 13), or newline (ASCII 10)
  LinuxTime.close();
  return SubDateTimeString;
}*/

// Save a byte of data to the SD card text file
String SaveStringtoSD(String SubStringtoSave) {

  // open the file. note that only one file can be open at a time, so you have to close this one before opening another.
  // The FileSystem card is mounted at the following "/mnt/FileSystema1"
  //DEBUG_PRINTLN(FilePath);
  //DEBUG_PRINT(F("Datafile: "));
  //DEBUG_PRINTLN(FileSystem.exists(FilePath.c_str()));

  File dataFile = FileSystem.open(FilePath.c_str(), FILE_APPEND);       //FILE_APPEND or FILE_READ or FILE_WRITE

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(SubStringtoSave);
    DEBUG_PRINT(F("File Size: "));
    DEBUG_PRINTLN(dataFile.size());     //return filesize (unsigned long) in bytes
    dataFile.close();
    // print to the console too:
    //DEBUG_PRINTLN(SubStringtoSave);
    DEBUG_PRINTLN(F("Saved"));
    //DEBUG_PRINT(F("File Exists: "));
    //DEBUG_PRINTLN(FileSystem.exists(FilePath.c_str()));
    return "Saved";
  }
  // if the file isn't open, pop up an error:
  else {
    DEBUG_PRINTLN("***error opening " + FilePath);
    RunScript(2,"***error opening");
    DEBUG_PRINT(F("Datafile: "));
    DEBUG_PRINTLN(dataFile);
    return "NotSaved";
  }
}

/*void CheckForClient(YunClient client){
  // There is a new server client?
  if (client) {
    String command = client.readString();         // read the command
    command.trim();        //kill whitespace
    //DEBUG_PRINTLN(command);
    if (command == "water") {                       //is "water" the command?  this "water" command comes from the "index.html" file on the SD card
      client.println(DateTimeString);                   //display the main content of the webpage
      DEBUG_PRINTLN(F("Sent to Client"));
    }
    else {
      Console.println(F("Wrong Command"));
    }
  }
  else {
      Console.println(F("No Client"));
  }
}*/

//flashes pin 13 light every second for the duration of the requested delay
void FlashLightDelay(unsigned long SubDelay) {
  unsigned long i = 0;
  //DEBUG_PRINTLN("FlashLightDelay Code");
  //DEBUG_PRINTLN(SubDelay);             //debug code
  for (i = 0; i < SubDelay;) {
    pinMode(13, OUTPUT);
    //DEBUG_PRINTLN(i);             //debug code
    if (digitalRead(13)) {
      digitalWrite(13, LOW);
      //DEBUG_PRINTLN("True");             //debug code
    }
    else {
      digitalWrite(13, HIGH);
      //DEBUG_PRINTLN("False");             //debug code
    }
    delay(1000);
    i = i + 1000;
  }
}

// this function will return the number of bytes currently free in RAM.  Written by David A. Mellis.  Based on code by Rob Faludi http://www.faludi.com
int availableMemory() {
  int size = 1024; // Use 2048 with ATmega328
  byte *buf;

  while ((buf = (byte *) malloc(--size)) == NULL);
  free(buf);

  return size;
}

void CheckConsoleText(){      //read text from console and save to log under "notes", use LEAST number of characters
  String Notes = "";
  Notes = "";       //clear any prior notes
  while(Console.available()) {
    //DEBUG_PRINTLN("Data Available");
    char NoteLetter = '!';      //i just picked an initial char that was not likely to be sent first
    NoteLetter = Console.read();
    //Console.println(NoteLetter,DEC);
    Notes = Notes + NoteLetter;
  }
  Notes.trim();
  if (Notes != "") {
    DateTimeString = RunScript(3,"");
    LogDataToSave = DateTimeString + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Comma + Notes;
    SaveStringtoSD(LogDataToSave);
  }
  //add if statements to allow commands to be sent to the Yun
}

String RunScript(int ScriptType, String IFAlertMsg){       //run any bash script
  String Output = "";
  Process Script;
  switch (ScriptType) {
    case 1:                     //get weather data
      Script.runShellCommand("/mnt/sd/weather.sh");
      while (Script.available()) {
        Output += (char)Script.read();
      }
      Output.trim();
      break;
    case 2:                     //send IFTTT Alert
      Script.runShellCommandAsynchronously("/mnt/sd/IFAlert.sh \""+IFAlertMsg+"\"");
      /*while (Script.available()) {
        //Console.println("Available");
        Output += (char)Script.read();
      }
      Output.trim();
      DEBUG_PRINTLN(Output);*/
      break;
    case 3:                     //get date and time data
      Script.runShellCommand("date +%Y%m%d%H%M%S");
      while (Script.available()) {
        Output += (char)Script.read();
      }
      Output.trim();
      break;
    case 4:                     //get date and time data
      Script.runShellCommand("/mnt/sd/PlotData.sh");
      while (Script.available()) {
        Output += (char)Script.read();
      }
      Output.trim();
      break;
  }
  //DEBUG_PRINTLN(Output);
  Script.close();
  return Output;
}

/*void IFAlert(String subIFMessage) {   //If This Than That function to send message to Cell Phone
  String SubAlertString;
  Process IFAlert;
  //curl option "-k" is needed since it is an https request.  This option ignores the https (insecure)
  IFAlert.runShellCommand("curl -m 10 -k -X POST -H \"Content-Type: application/json\" -d '{\"value1\":\"" + subIFMessage + "\"}' https://maker.ifttt.com/trigger/WaterTower/with/key/6V2eDdg75RHJjlk7FTFzG");
  //while (IFAlert.available()) {
    //char c = IFAlert.read();
    //SubAlertString += c;
  //}
  //DEBUG_PRINTLN(SubAlertString);
  IFAlert.close();
}*/

/*void SavetoConfig(String SubStringforConfig) {

  // open the file. note that only one file can be open at a time, so you have to close this one before opening another.
  // The FileSystem card is mounted at the following "/mnt/FileSystema1"
  //DEBUG_PRINTLN(FilePath);
  //DEBUG_PRINT(F("Datafile: "));
  //DEBUG_PRINTLN(FileSystem.exists(FilePath.c_str()));

  File dataFile = FileSystem.open("/mnt/sd/config", FILE_WRITE);       //FILE_APPEND or FILE_READ or FILE_WRITE

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(SubStringtoSave);
    DEBUG_PRINT(F("File Size: "));
    DEBUG_PRINTLN(dataFile.size());     //return filesize (unsigned long) in bytes
    dataFile.close();
    // print to the console too:
    //DEBUG_PRINTLN(SubStringtoSave);
    DEBUG_PRINTLN(F("Saved"));
    //DEBUG_PRINT(F("File Exists: "));
    //DEBUG_PRINTLN(FileSystem.exists(FilePath.c_str()));
    return "Saved";
  }
  // if the file isn't open, pop up an error:
  else {
    Console.println("***error opening " + FilePath);
    DEBUG_PRINT(F("Datafile: "));
    DEBUG_PRINTLN(dataFile);
    return "NotSaved";
  }
}*/

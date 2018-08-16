/*Device Features:

 (1) Wifi Manager
 (2) Watch Dog
 (3) Manual control(on/off) of pump and fan
 (4) Based on set temp threshold turn on/off AC
 (5) Water sparay @ 5 user defined time (times are written in EEPROM)
 (6) Generate different topic for different user
 (7) RTC to read current time; adjust RTC time with server while required
 (8) Read hum,temp,co2,light intensity and publish @ defined interval 
 (9) Display humidity,temperature and water spray time on OLED display

 To Do:

 (1) Add OTA
 (2) Add offline mode to spray water @ given time and show current status (hum,temp,water spray
 time) when internet is unavailable.
 
 (3) While did,uid empty MQTT connected and disconnected contineously: solution:I subscribed to empty topics
 before constructing the topics
Device is subscribed to:

Topic----------------------------------------------------Msg.

When no did and uid assigned only then subscribed to

 mushroom/configure===================uid,did

(1) mushroom/uid/did/user_input=========2(pump on),3(pump off),4(fan off),5(fan on)
(2)  mushroom/uid/did/time=============00:00,00:00,00:00,00:00,00:00
(3) mushroom/uid/did/chk-status========1
(4) mushroom/uid/did/temp=============temerature threshold (an integer)
(5) mushroom/uid/did/reset============1
(6) mushroom/uid/did/deviceTime=======1



Device is publishing To:

Topic-----------------------------------------------------Msg.

mushroom/uid/did/sensor_data========================hum,temp,co2,light
mushroom/uid/did/fan_status=========================on/off
mushroom/uid/did/ack=================================1 (after water spray for 1 minutes)

mushroom/uid/did/currentTime========================publishig current time read from RTC
mushroom/uid/did/configack===========================yes when device configuration is done
mushroom/uid/did/pump_status=========================on/off

*/


/*Comment:
 * 
 * While integrating I found that when watchdog feature is enabled D15 pin 
 * is used by the watchdog library and thus MQTT indicator high/low frequently.
 * I've changed the pin from D15 to D23
 * 
 */

//-----------------------------Including required libraries---------------------------------------------------//
#include "SPIFFS.h"
#include <time.h> // Library to read time from server
#include "RTClib.h"
RTC_DS3231 rtc;
#include "DHTesp.h"             //library: https://github.com/beegee-tokyo/DHTesp
#include <WiFi.h>
#include <PubSubClient.h>
#include "EEPROM.h"
#include <DNSServer.h>
//#include <ESP8266WebServer.h>
//#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include <ESP32Ticker.h>
#include <Wire.h>

Ticker secondTick;

//https://github.com/bblanchon/ArduinoJson
char* ssid="DataSoft_WiFi";
char* password="support123";

//MQTT  credentials

const char* mqtt_server = "182.163.112.207";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);



//-----------------------------Defining required pins-------------------------------------------------------//

#define light_sensor 34
#define dht_dpin 26
#define co2_sensor 35 
#define ACpin 12
#define Pumppin 13
#define Fanpin 14
#define WiFiPin 2
#define MQTTPin 23
#define configPin 5
#define DHTTYPE DHT22
//Defining EEPROM size
#define EEPROM_SIZE 200

int deviceOnline=0;


long data_publishing_interval=300000;
long clock_reading_interval=50000;


//-----------------------Real time clock (from server)-----------------------------------//

String Clock="";
String h="";
String m="";
int hour_length;
int minute_length;
int timezone = 6; //Timezone for Dhaka UTC+6
int adjustedHour;
int adjustedMinute;

//----------------------Real time clock (from module)-------------------------------------//

String Clock2="";
String h2="";
String m2="";
int hour_length2;
int minute_length2;


//----------------User given time spliting variables------------------------------------//

char Data[30];
const int numberOfPieces = 5;
String t[numberOfPieces];
String input;
//Keep track of current position in time array
int counter = 0;

// Keep track of the last comma so we know where to start the substring
int lastIndex = 0;

char Time0[6];
char Time1[6];
char Time2[6];
char Time3[6];
char Time4[6];


//int clock_usertime_matched_flag=0;


//-------------------Storing temperature threshold from user end--------------------------------------------//

int tempThreshold;
char tempThresholdData[6];


//String input2;
//char Data2[6];
char Data3[6];
//String msg2;

//------------------------Storing Hardware Status----------------------------//

String fanStatus="off";
String pumpStatus="off";
String acStatus="0";


//----------------------------OLED display----------------------------------//

//#include <Adafruit_SSD1306.h>
//#include <Adafruit_GFX.h>

// OLED display TWI address

//#define OLED_ADDR   0x3C


//Adafruit_SSD1306 display(-1);// no reset pin for OLED



//Variable to keep config data (uid,did) read and write address on EEPROM

int configDataReadAddress=0;
int configDataWriteAddress=0;

//Variable to keep water spray time read and write address on EEPROM
int spraytimeReadAddress=100;
int spraytimeWriteAddress=100;

//Variable to keep tempThreshodData read and write address on EEPROM


int tempThresholdReadAddress=130;
int tempThresholdWriteAddress=130;


//-------------------------Topic container--------------------------------------------//

char topic0[50]="";
char topic1[50]="";
char topic2[50]="";
char topic3[50]="";
char topic4[50]="";
char topic5[50]="";
//char topic6[50]="";
//char topic7[50]="";
char topic8[50]="";
char topic9[50]="";
char topic10[50]="";
char topic11[50]="";
char topic12[50]="";
char topic13[50]="";
char topic14[50]="";
//------------------------Defining common portion of the topics------------------------//

char projectName[]="mushroom/";

//------------------------Subscribed topics--------------------------------------------//
char commonTopic0[50]="/user_input";
char commonTopic1[50]="/temp";
char commonTopic2[50]="/time";
char commonTopic3[50]="/chk-status";
char commonTopic4[50]="/deviceTime";
char commonTopic5[50]="/reset";
//char commonTopic6[50]="/clearEEPROM";
//char commonTopic7[50]="/checkEEPROM";

//------------------------Publishing topics--------------------------------------------//
char commonTopic8[50]="/sensor_data";
char commonTopic9[50]="/fan_status";
char commonTopic10[50]="/ack";
char commonTopic11[50]="/online";
char commonTopic12[50]="/currentTime";
char commonTopic13[50]="/configAck";
char commonTopic14[50]="/pump_status";


//----------------deviceid & userid spliting variables------------------------------------//

char uniqueData[30];
const int configDataPieces = 2;
String configData[configDataPieces];
String did_uid="";
// Keep track of current position in configData array
int configDataCounter = 0;

// Keep track of the last comma so we know where to start the substring
int configDataLastIndex;

char did[24]={'0'};
char uid[24]={'0'};


//Sensor data storing variable:

DHTesp dht;
int data1=0;
int data2=0;
int data3=0;
int data4=0;
unsigned long lastMsg = 0;
unsigned long previousMillis = 0;



//------------------------WATCH DOG SUBROUTINE-------------------------------//

//int watchdogCount=0;
volatile int watchdogCount=0;
void ISRwatchdog(){


  watchdogCount++;
  if(watchdogCount==100){


    //Serial.println();
    Serial.print("The watch dog bites......");
    ESP.restart();
  }
}



//--------------------------------------------Main Set up----------------------------------------------//


void setup() {
  
  
  
  // put your setup code here, to run once:

  secondTick.attach(1,ISRwatchdog);
  
  Serial.begin(115200);
  Serial.println("");

  //initializing indicator led pin
  
  
  pinMode(ACpin,OUTPUT);
  pinMode(Pumppin,OUTPUT);
  pinMode(Fanpin,OUTPUT);
  pinMode(WiFiPin,OUTPUT);
  pinMode(MQTTPin,OUTPUT);
  pinMode(configPin,OUTPUT);

  
  digitalWrite(ACpin,LOW);
  digitalWrite(Pumppin,LOW);
  digitalWrite(Fanpin,LOW);
  digitalWrite(WiFiPin,LOW);
  digitalWrite(MQTTPin,LOW);
  digitalWrite(configPin,LOW);

  //initialize sensor reading pins
  
  dht.setup(dht_dpin,DHTesp::DHTTYPE);
  pinMode(light_sensor,INPUT);
  pinMode(dht_dpin,INPUT);
  pinMode(co2_sensor,INPUT);
  
  initialize_EEPROM();
  setup_wifi();
  config_RTC();


  client.setServer(mqtt_server,mqttPort);
  client.setCallback(callback);

}


//-------------------------------------------Main Loop--------------------------------------------------//

void loop() {
  // put your main code here, to run repeatedly:

   watchdogCount=0;
  //Serial.println(watchdogCount);
  
  if (!client.connected()) {
    reconnect();
  }

  
  
  client.loop();

//-----------Reading SensorData--------------//
  
  
  readSensorData();
  


    if (data1<50){

/*     display_hum();
       delay(500);
       display_temp();
       delay(500);
       display_water_spray_time();
       delay(2000);
  */
       sensor_data_publish();
      //Controlling AC based on user threshold
      ac_Control();
      

 
}
//------------------------------------------Reading clock @ every 50s interval---------------------------//


  unsigned long currentMillis=millis();
  if(currentMillis-previousMillis>clock_reading_interval)
  {
  previousMillis=currentMillis;  
  actual_time_rtc();
  delay(500); 
  
  
  Serial.print("Actual time:");
  Serial.println(Clock2);

  readingSprayTimeFromEEPROM();
  }


  //turn on pump & fan for 1 min while 
  //current time==spray time

   waterSpray();
  } //End of main loop

//-----------------------------------------------Reading Sensor Data------------------------------//


void readSensorData(){

  data1=temp();
  data2=hum();
  delay(200);
  data3=co2();
  data4=light();

}

  

//------------------------------------------------While client not conncected---------------------------------//

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if your MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      digitalWrite(MQTTPin,HIGH);//lighting up MQTT indicator
      deviceOnline=1;
      
      
     
     //once connected to MQTT broker, subscribe command if any
     //----------------------Subscribing to required topics-----------------------//

     


                  
                 readConfigDatafromEEPROM();

                  if(strlen(did)==0 && strlen(uid)==0){
                    
                    client.subscribe("mushroom/configure");
                    Serial.println("subscribed to mushroom/configure");
                    digitalWrite(configPin,LOW);
                  }
                  
                  if(strlen(did)!=0 && strlen(uid)!=0){

                    client.unsubscribe("mushroom/configure");
                    
                    
                    
                   
                    Serial.println("You already have did and uid assigned");
                    Serial.println("Unsubscribed from: mushroom/configure");
                    
                    Serial.print("did:");
                    Serial.println(did);
                    Serial.print("uid:");
                    Serial.println(uid);
                    

//----------------------------------Constructing topics here----------------------------------------------------//
                      
                      strcat(projectName,uid);
                      strcat(projectName,"/");
                      strcat(projectName,did);
                      Serial.print("projectName/uid/did/:");
                      Serial.println(projectName);

//--------------------------Publishing ack while device configured----------------------------------------------//


                          strcpy(topic13,projectName);
                          strcat(topic13,commonTopic13);
                          Serial.print(":");
                          Serial.println(topic13);

                          digitalWrite(configPin,HIGH);//lighting up config indicator
                          client.publish(topic13,"yes");                      
                          constructTopic();

                          //-----------------------------Subscribing to required topics----------------------------------------//

                          
                        
                          client.subscribe(topic0);
                          Serial.println("Subscribed to /user_input"); 
                          client.subscribe(topic1);
                          Serial.println("Subscribed to /temp");
                          client.subscribe(topic2);
                          Serial.println("Subscribed to /time");
                          client.subscribe(topic3);
                          Serial.println("Subscribed to /chk-status");
                          client.subscribe(topic4);
                          Serial.println("Subscribed to /deviceTime");
                          client.subscribe(topic5);
                          Serial.println("Subscribed to /reset");
                      

                      
                  }



      
      
            
      } else {

     
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(MQTTPin,LOW);
      deviceOnline=0;
      
      
      //Doing offline duty (water spray and ac control)

      readSensorData();
      ac_Control();
      actual_time_rtc();
      waterSpray();
      
      
      
      // Wait 6 seconds before retrying
      
      delay(5000);
    }
  }
}




//-----------------------Callback function-------------------------------------//


void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  
//-------------------------------------Getting config data---------------------//

if(strcmp(topic, "mushroom/configure") == 0){


  Serial.print("Message:");
  memset(Data, 0, sizeof(Data)/sizeof(char));//Resetting the array with all zeros
  Serial.print("Data array size:");
  Serial.println(sizeof(Data)/sizeof(char));
  Serial.print("Data before getting msg:");
  Serial.println(Data);

  
   


    for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    //input=input+payload[i];
    uniqueData[i]=payload[i];
    }
      Serial.print("uniqueData:");
      Serial.println(uniqueData);
      did_uid=did_uid+uniqueData;
      Serial.print("Device id,User id:");
      Serial.print(did_uid);
      Serial.println();
      // Loop through each character and check if it's a comma
      Serial.print("configData length:");
      Serial.println(did_uid.length());
      configDataLastIndex =0;
      for (int i = 0;i < did_uid.length(); i++){  
        
        if (did_uid.substring(i, i+1) == ",") //input.substring: This method extracts the characters in a string 
                                            //between "start" and "end", not including "end" itself
        {
          // Grab the piece from the last index up to the current position and store it
          Serial.print("configDataLastIndex:");
          Serial.println(configDataLastIndex);
          configData[configDataCounter] = did_uid.substring(configDataLastIndex, i);
          Serial.println(configData[configDataCounter]);
          // Update the last position and add 1, so it starts from the next character
          configDataLastIndex = i + 1;
          // Increase the position in the array that we store into
          configDataCounter++;
        }

        // If we're at the end of the string (no more commas to stop us)
        Serial.print("last string start at:");
        Serial.println(i);
        if (i == did_uid.length()-1) {
          // Grab the last part of the string from the lastIndex to the end
          configData[configDataCounter] = did_uid.substring(configDataLastIndex, i+1);
          Serial.println(configData[configDataCounter]);
        }}
         
      // Clear out string and counters to get ready for the next incoming string
      did_uid ="";
      configDataCounter = 0;
      configDataLastIndex = 0;
      // Writing did and uid to EEPROM
      writeConfigDataToEEPROM();
          
            
      }


//if(strcmp(topic, "mushroom/chk-status") == 0){
if(strcmp(topic,topic3) == 0){
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    char data=payload[i];
    
  
      if (data=='1')
    {
      char Status1[5]={};
      char Status2[5]={};


      pumpStatus.toCharArray(Status1,6);
      fanStatus.toCharArray(Status2,6);
      Serial.print("Pump Status:");
      Serial.println(Status1);
      Serial.print("Fan Status:");
      Serial.println(Status2);
      client.publish(topic9,Status2);
      client.publish(topic14,Status1);
      }
  }}



//------------------------user_input for manual load control-----------------//


//  if(strcmp(topic, "mushroom/user_input") == 0){
if(strcmp(topic,topic0) == 0){
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    char data=payload[i];
    
  
      if (data=='2')
    {
      digitalWrite(Pumppin,HIGH);
      Serial.println("Pump ON");
      pumpStatus="on";
      
      }
      if (data=='3')
    {
      digitalWrite(Pumppin,LOW);
      Serial.println("Pump OFF");
      pumpStatus="off";
      }
      if (data=='4')
    {
      digitalWrite(Fanpin,LOW);
      Serial.println("Fan OFF");
      fanStatus="off";
      
      }
      if (data=='5')
    {
      digitalWrite(Fanpin,HIGH);
      Serial.println("Fan ON");
      fanStatus="on";
      }
       
  }}
  

   //-------------------------------Getting temperature threshold from user--------------------------------//


//      if(strcmp(topic, "mushroom/temp") == 0)

      if(strcmp(topic,topic1) == 0)
      {
        memset(tempThresholdData,0, sizeof(tempThresholdData));// Emptying the char array
        Serial.print("Message:");
        for (int i = 0; i < length; i++) {
        tempThresholdData[i]=payload[i];
        
    }
    Serial.println(tempThresholdData);
    writeTempThresholdToEEPROM();
    
//    tempThreshold=atoi(tempThresholdData);// Converting char array to int
//    Serial.println(tempThreshold);
//    
    }
   
   
   //------------------------------Getting time from user end-------------------------//


//if(strcmp(topic, "mushroom/time") == 0){

if(strcmp(topic,topic2) == 0){

  
  Serial.print("Message:");
  memset(Data, 0, sizeof(Data));//Resetting the array with all zeros
  Serial.print("Data array size:");
  Serial.println(sizeof(Data));
  Serial.print("Data before getting msg:");
  Serial.println(Data);
  
  
  t[0]="00:00";
  t[1]="00:00";
  t[2]="00:00";
  t[3]="00:00";
  t[4]="00:00";
  for (int i = 0; i < length; i++) {
   //Serial.print((char)payload[i]);
    //input=input+payload[i];
    Data[i]=payload[i];
    }
      Serial.print("Data:");
      Serial.println(Data);
      input=input+Data;
      Serial.print("input:");
      Serial.print(input);
      // Loop through each character and check if it's a comma
      Serial.print("input length:");
      Serial.println(input.length());
       for (int i = 0; i < input.length(); i++){  
        
        if (input.substring(i, i+1) == ",") //input.substring: This method extracts the characters in a string 
                                            //between "start" and "end", not including "end" itself
        {
          // Grab the piece from the last index up to the current position and store it
          t[counter] = input.substring(lastIndex, i);
          Serial.println(t[counter]);
          // Update the last position and add 1, so it starts from the next character
          lastIndex = i + 1;
          // Increase the position in the array that we store into
          counter++;
        }

        // If we're at the end of the string (no more commas to stop us)
        Serial.print("last string start at: ");
        Serial.println(i);
        if (i == input.length()-1) {
          // Grab the last part of the string from the lastIndex to the end
          t[counter] = input.substring(lastIndex, i+1);
          Serial.println(t[counter]);
        }}
         
      // Clear out string and counters to get ready for the next incoming string
      input = "";
      counter = 0;
      lastIndex = 0;
      time_from_user_end();
    }


//---------------------------Reset the board from Engineer's end----------------------------//

//if(strcmp(topic, "mushroom/reset") == 0){

if(strcmp(topic,topic5) == 0){
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    char data5=payload[i];
    
  
    if (data5=='1')
    {
      
      Serial.println("Resetting NodeMCU.........");
      ESP.restart();
      }}}

//if(strcmp(topic, "mushroom/deviceTime") == 0){

if(strcmp(topic,topic4) == 0){
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.println((char)payload[i]);
    char data6=payload[i];
    Serial.println(data6);
  
    if (data6=='1')
    {

      
      //Serial.print(" I'm here ");
      Clock2.toCharArray(Data3,6);
      //client.publish("mushroom/currentTime",Data3);
      client.publish(topic12,Data3);
      Serial.print("Current time is:");
      Serial.println(Data3);
 
      }
      }
      }
      }

  
  
  


//--------------------------------------------------WiFi Setup-------------------------------------//

void setup_wifi() {

   // WiFiManager wifiManager;
   //wifiManager.resetSettings();//clear previosly saved ssid,password

//    if (!wifiManager.autoConnect("Mushroom Device", "admin1234")) {
//    Serial.println("failed to connect and hit timeout");
//    delay(3000);
//    ESP.restart();
//    delay(5000);
//    digitalWrite(WiFiPin,LOW);

//  }
//  Serial.println("connected");
//  digitalWrite(WiFiPin,HIGH);// lighting up wifi indicator

 delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
//      deviceOnline=0;
      //Doing offline duty (water spray and ac control)

//      readSensorData();
//      ac_Control();
//      actual_time_rtc();
//      waterSpray();
//      
      
}
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
     //deviceOnline=1;
}

//----------------------------------RTC config-----------------------------------------------------------//

void config_RTC(){

    if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  
}

configTime(timezone * 3600, 0, "bd.pool.ntp.org", "bsti1.time.gov.bd");

//for(int i=0;i<9;++i){
//actual_time_server();
//delay(1000);
//}
//adjustedHour=h.toInt();
//adjustedMinute=m.toInt();

// Setting up RTC for the first time
//rtc.adjust(DateTime(2018, 8, 16, 15,38, 00));

}

//-------------------------Getting actual time from server-----------------------------------------------//

void actual_time_server(){

  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now);
  
  h= timeinfo->tm_hour;
  m= timeinfo->tm_min;
  
  hour_length=h.length();
  minute_length=m.length();
  if(hour_length==1){
  h="0"+h;
  }
  if(minute_length==1){
  m="0"+m;
 }
  
  

  Clock=h+":"+m;
  Serial.print("Server Time:");
  Serial.println(Clock);
 
}

//---------------------------Reading current time from RTC------------------------------------------------------//


void actual_time_rtc(){


  DateTime now=rtc.now();
  
  h2=String(now.hour(),DEC);
  
  hour_length2=h2.length();
  if(hour_length2==1){
  h2="0"+h2;
  }
  
  m2=String(now.minute(),DEC);
  minute_length2=m2.length();
  if(minute_length2==1){
  m2="0"+m2;
  }  
  
  
  Clock2=h2+":"+m2;

  Serial.print("Clock2:");
  Serial.println(Clock2);
  
  
  if(Clock2=="165:165"){
  for(int i=0;i<5;++i){
          actual_time_server();
          delay(1000);
          }
    adjustedHour=h.toInt();
    adjustedMinute=m.toInt();
    rtc.adjust(DateTime(2018, 7, 17, adjustedHour, adjustedMinute, 00));
   
    }
  
  
delay(1000);  
 }






//-----------------------------------EEPROM initialization ---------------------------------------------//

void initialize_EEPROM(){

  Serial.println("\nInitializing EEPROM library\n");
  if (!EEPROM.begin(EEPROM_SIZE)) 
  {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  //-----------Clearing EEPROM; uncomment the following loop if u want to clear EEPROM-------------------------//

//    for (int i = 0 ; i < EEPROM_SIZE ; i++) {
//    EEPROM.write(i, 0);
//  }
//EEPROM.commit();

}

//----------------------------------------------Time from user end------------------------------------------//


void time_from_user_end(){

  
 memset(Time0,0,sizeof(Time0));
 memset(Time1,0,sizeof(Time1));
 memset(Time2,0,sizeof(Time2));
 memset(Time3,0,sizeof(Time3));
 memset(Time4,0,sizeof(Time4));
  
  
  t[0].toCharArray(Time0,6);
  Serial.print("Time0:");
  Serial.println(Time0);
  t[1].toCharArray(Time1,6);
  Serial.print("Time1:");
  Serial.println(Time1);
  //Serial.print("t[2]");
  //Serial.println(t[2]);
  t[2].toCharArray(Time2,6);
  Serial.print("Time2:");
  Serial.println(Time2);
  t[3].toCharArray(Time3,6);
  Serial.print("Time3:");
  Serial.println(Time3);
  t[4].toCharArray(Time4,6);
  Serial.print("Time4:");
  Serial.println(Time4);

  writeSprayTimeToEEPROM();
  
}


//------------------Writing spray time to EEPROM--------------------------------------------//

void writeSprayTimeToEEPROM(){

  spraytimeWriteAddress=100;

  //Serial.println("Elements in Time0");
  //Serial.println(sizeof(Time0)/sizeof(char));
  for(int i=0;i<sizeof(Time0)/sizeof(char);++i){

    EEPROM.write(spraytimeWriteAddress,Time0[i]);
    //Serial.print("Current address:");
    //Serial.println(spraytimeWriteAddress);
    spraytimeWriteAddress++;
    }
    Serial.print("Current address after Time0:");
    Serial.println(spraytimeWriteAddress);

  for(int i=0;i<sizeof(Time1)/sizeof(char);++i){

    //addr+=i;
    EEPROM.write(spraytimeWriteAddress,Time1[i]);
    //Serial.print("Current address:");
    //Serial.println(spraytimeWriteAddress);
    spraytimeWriteAddress++;
    }

    Serial.print("Current address after Time1:");
    Serial.println(spraytimeWriteAddress);

  for(int i=0;i<sizeof(Time2)/sizeof(char);++i){
    
    EEPROM.write(spraytimeWriteAddress,Time2[i]);
    //Serial.print("Current address:");
   // Serial.println(spraytimeWriteAddress);
    spraytimeWriteAddress++;
    }

    Serial.print("Current address after Time2:");
    Serial.println(spraytimeWriteAddress);

for(int i=0;i<sizeof(Time3)/sizeof(char);++i){

    
    EEPROM.write(spraytimeWriteAddress,Time3[i]);
    //Serial.print("Current address:");
    //Serial.println(spraytimeWriteAddress);
    spraytimeWriteAddress++;
    }

    Serial.print("Current address after Time3:");
    Serial.println(spraytimeWriteAddress);

for(int i=0;i<sizeof(Time4)/sizeof(char);++i){

    
    EEPROM.write(spraytimeWriteAddress,Time4[i]);
    //Serial.print("Current address:");
    //Serial.println(spraytimeWriteAddress);
    spraytimeWriteAddress++;
    }
    
    Serial.print("Current address after Time4:");
    Serial.println(spraytimeWriteAddress);


    EEPROM.commit();
    
}

//-------------------------------Reading spray time from EEPROM---------------------------------------------//


void readingSprayTimeFromEEPROM()

{

 Serial.println("");
 spraytimeReadAddress=100;
 memset(Time0,0,sizeof(Time0));
 memset(Time1,0,sizeof(Time1));
 memset(Time2,0,sizeof(Time2));
 memset(Time3,0,sizeof(Time3));
 memset(Time4,0,sizeof(Time4));
 
  
 for(int i=0;i<sizeof(Time0)/sizeof(char);++i)
 {

  
  //addr+=i;
  Time0[i]=char(EEPROM.read(spraytimeReadAddress));
  spraytimeReadAddress++;  
    
  }
  
  Serial.print("Time0:");
  Serial.println(Time0);
  Serial.print("Current address:");
  Serial.println(spraytimeReadAddress);

 for(int i=0;i<sizeof(Time1)/sizeof(char);++i){

  //addr+=i;
  Time1[i]=char(EEPROM.read(spraytimeReadAddress));
  spraytimeReadAddress++;  
    
  }
 Serial.print("Time1:");
 Serial.println(Time1);
 Serial.print("Current address:");
 Serial.println(spraytimeReadAddress);

 for(int i=0;i<sizeof(Time2)/sizeof(char);++i){

  //addr+=i;
  Time2[i]=char(EEPROM.read(spraytimeReadAddress));
  spraytimeReadAddress++;  
    
  }
  Serial.print("Time2:");
  Serial.println(Time2);
  Serial.print("Current address:");
  Serial.println(spraytimeReadAddress);

 for(int i=0;i<sizeof(Time3)/sizeof(char);++i){

  //addr+=i;
  Time3[i]=char(EEPROM.read(spraytimeReadAddress));
  spraytimeReadAddress++;  
    
  }

  Serial.print("Time3:");
  Serial.println(Time3);
  Serial.print("Current address:");
  Serial.println(spraytimeReadAddress);

 for(int i=0;i<sizeof(Time4)/sizeof(char);++i){

  //addr+=i;
  Time4[i]=char(EEPROM.read(spraytimeReadAddress));
  spraytimeReadAddress++;
    
    
  }
  Serial.print("Time4:");
  Serial.println(Time4);
  Serial.print("Current address:");
  Serial.println(spraytimeReadAddress);



}



//--------------------------------------- Writing did and uid to EEPROM-----------------------------------------//


void writeConfigDataToEEPROM()

{

EEPROM.writeString(configDataWriteAddress,configData[0]);
Serial.print("Size of configData[0]:");
Serial.println(configData[0].length());

// Updating address for configData[1]
            
configDataWriteAddress+=10;
EEPROM.writeString(configDataWriteAddress,configData[1]);
Serial.println(configData[1].length());

EEPROM.commit();

ESP.restart();

}            

// ----------------Reading did and uid from EEPROM-----------------------------//


void readConfigDatafromEEPROM()
{

  memset(did,0,sizeof(did));
  memset(uid,0,sizeof(uid));
 
  
  for(int i=0;i<10;++i){


    uid[i]=char(EEPROM.read(configDataReadAddress));
    configDataReadAddress++;
}

  Serial.print("did:");
  Serial.println(did);

  for(int j=0;j<10;++j){

       
      did[j]=char(EEPROM.read(configDataReadAddress));
      configDataReadAddress++;
    
  }

  Serial.print("did:");
  Serial.println(did);
  }     


//------------------------------------Constructing topics-----------------------------------------------------//

void constructTopic(){

strcpy(topic0,projectName);
strcat(topic0,commonTopic0);
Serial.print("topic0:");
Serial.println(topic0);


strcpy(topic1,projectName);
strcat(topic1,commonTopic1);
Serial.print("topic1:");
Serial.println(topic1);


strcpy(topic2,projectName);
strcat(topic2,commonTopic2);
Serial.print("topic2:");
Serial.println(topic2);

strcpy(topic3,projectName);
strcat(topic3,commonTopic3);
Serial.print("topic3:");
Serial.println(topic3);

strcpy(topic4,projectName);
strcat(topic4,commonTopic4);
Serial.print("topic4:");
Serial.println(topic4);

strcpy(topic5,projectName);
strcat(topic5,commonTopic5);
Serial.print("topic5:");
Serial.println(topic5);

//strcpy(topic6,projectName);
//strcat(topic6,commonTopic6);
//Serial.print("topic6:");
//Serial.println(topic6);
//
//strcpy(topic7,projectName);
//strcat(topic7,commonTopic7);
//Serial.print("topic7:");
//Serial.println(topic7);

strcpy(topic8,projectName);
strcat(topic8,commonTopic8);
Serial.print("topic8:");
Serial.println(topic8);

strcpy(topic9,projectName);
strcat(topic9,commonTopic9);
Serial.print("topic9:");
Serial.println(topic9);

strcpy(topic10,projectName);
strcat(topic10,commonTopic10);
Serial.print("topic10:");
Serial.println(topic10);

strcpy(topic11,projectName);
strcat(topic11,commonTopic11);
Serial.print("topic11:");
Serial.println(topic11);

strcpy(topic12,projectName);
strcat(topic12,commonTopic12);
Serial.print("topic12:");
Serial.println(topic12);


strcpy(topic13,projectName);
strcat(topic13,commonTopic13);
Serial.print("topic13:");
Serial.println(topic13);

strcpy(topic14,projectName);
strcat(topic14,commonTopic14);
Serial.print("topic14:");
Serial.println(topic14);

//ESP.restart();
}



//-------------------------------Reading Sensor Data--------------------------------------------//


int temp()
{
  
  //int t = dht.readTemperature(); 
  int t=dht.getTemperature();
  Serial.print("Temperature:");
  Serial.println(t);        
  return t;
   
  }

int hum()
{
  //int h = dht.readHumidity();
  int h=dht.getHumidity();
//  Serial.print("Humidity:");
//  Serial.println(h);
  return h;
  
  }

int co2(){

int co2now[10];//long array for co2 readings
int co2raw=0;  //long for raw value of co2
int co2comp = 0;   //long for compensated co2 
int co2ppm = 0;    //long for calculated ppm
int sum=0;
for (int x=0;x<10;x++){

co2now[x]=analogRead(co2_sensor);
sum=sum+co2now[x];
}
co2raw=sum/10;
co2raw=co2raw-55;
co2ppm=map(co2raw,0,4095,300,2000);

//Serial.print("co2 in ppm:");
//Serial.println(co2ppm);
return co2ppm; 
  
}


int light(){
  
  int l = analogRead(light_sensor);
  int light_intensity=map(l,0,4095,80,400);
//  Serial.print("light:");
//  Serial.println(light_intensity);
  return light_intensity;
}


//------------------------------Publishing sensor data every 5 minutes----------------------------------//


void sensor_data_publish(){
  //Serial.println("I'm in sensor_data_publish");
  unsigned long now=millis();
  if(now-lastMsg>data_publishing_interval){
    
    lastMsg=now;
    String msg=""; 
    msg= msg+ data2+","+data1+","+data3+","+data4;
    char message[68];
    msg.toCharArray(message,68);
    Serial.println(msg);
    Serial.println(message);
    //client.publish("mushroom/sensor_data",message);
    client.publish(topic8,message);

    
  }}

//-----------------------------AC control---------------------------------------------//


void ac_Control(){
  
 
 
 readingTempThresholdFromEEPROM();
 
 if(data1>tempThreshold && acStatus=="0")
 {
  Serial.println("AC On");
  digitalWrite(ACpin,HIGH);
  acStatus="1";
 } 
 
 if (data1<tempThreshold && acStatus=="1")

  {
    Serial.println("AC off");
    digitalWrite(ACpin,LOW);
    acStatus="0";
  }
  
  }//end of ac_Control


//-------------------Turning on pump & fan for 1 min while spray time == current time-----------------------//

void waterSpray(){
 //--------------------------------USER TIME & ACTUAL TIME COMPARISON--------------------------//
 
 if (Clock2!=""){
// if ((Clock2 == Time0 || Clock2 == Time1|| Clock2 == Time2 || Clock2 == Time3 || Clock2 == Time4)&& clock_usertime_matched_flag==0)
  if (Clock2 == Time0 || Clock2 == Time1|| Clock2 == Time2 || Clock2 == Time3 || Clock2 == Time4)
  
  {
    //clock_usertime_matched_flag=1;
    digitalWrite(Pumppin,HIGH);
    Serial.println("Pump on");
    pumpStatus="on";
    
    //client.publish("mushroom/ack","1");

    digitalWrite(Fanpin,HIGH);
    Serial.println("Fan on");
    fanStatus="on";
    //client.publish("mushroom/ack","2");

    if(deviceOnline==1)
    client.publish(topic10,"1");
    
    delay(60000);//use millis here 
    
    
    digitalWrite(Pumppin,LOW);
    digitalWrite(Fanpin,LOW);
    

    //client.publish("mushroom/ack","Pump off");
    //client.publish("mushroom/ack","Fan off");
    pumpStatus="off";
    fanStatus="off";
  }
  }}



//----------Displaying temp and hum on OLED-------------------------//

//void display_hum(){
// 
//  display.clearDisplay();
//  
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  display.setCursor(10,0);
//  display.println("Humidity");
//  display.setCursor(50,15);
//  display.print(data2);
//  display.print("%");
//  display.display();
// 
//  }
//
//void display_temp(){
//
//   display.clearDisplay();
//   display.setTextSize(2);
//   display.setTextColor(WHITE);
//   display.setCursor(0,0);
//   display.print("Temp ");
//   //display.setCursor(50,15);
//   display.print(data1);
//   display.print((char)247);
//   display.println("C");
//   display.display();
//   
//}
//
//void display_water_spray_time()
//{
//  
//
//   display.clearDisplay();
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   display.setCursor(0,0);
//   display.println("Water Spray Time:");
//   display.print(Time0);
//   display.print(",");
//   display.print(Time1);
//   display.println(",");
//   display.print(Time2);
//   display.print(",");
//   display.print(Time3);
//   display.println(",");
//   display.print(Time4);
//   display.display();
//   //delay(10000);
//
//}
//
//------------------Writing temperature threshold to EEPROM--------------------------------------------//

void  writeTempThresholdToEEPROM(){

  tempThresholdWriteAddress=130;

  for(int i=0;i<sizeof(tempThresholdData)/sizeof(char);++i){

    EEPROM.write(tempThresholdWriteAddress,tempThresholdData[i]);
    //Serial.print("Current address:");
    //Serial.println(spraytimeWriteAddress);
    tempThresholdWriteAddress++;
    }
    Serial.print("Current address after tempThreshold:");
    Serial.println(tempThresholdWriteAddress);
    EEPROM.commit();
    
}

//-------------------------------Reading spray time from EEPROM---------------------------------------------//


void readingTempThresholdFromEEPROM()

{

 Serial.println("");
 tempThresholdReadAddress=130;
 memset(tempThresholdData,0,sizeof(tempThresholdData));
 
  
 for(int i=0;i<sizeof(tempThresholdData)/sizeof(char);++i)
 {

  
  //addr+=i;
  tempThresholdData[i]=char(EEPROM.read(tempThresholdReadAddress));
  tempThresholdReadAddress++;  
    
  }

  
  tempThreshold=atoi(tempThresholdData);// Converting char array to int
    
  Serial.print("tempThreshold:");
  Serial.println(tempThreshold);
  
}


 

#include <WiFi.h>
#include <PubSubClient.h>
#define EEPROM_SIZE 200
#include "EEPROM.h"

int configDataReadAddress=0;
int configDataWriteAddress=0;


char* ssid="DataSoft_WiFi";
char* password="support123";
const char* mqtt_server = "182.163.112.207";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

//-------------------------Topic container--------------------------------------------//

char topic0[50]="";
char topic1[50]="";
char topic2[50]="";
char topic3[50]="";
char topic4[50]="";
char topic5[50]="";
char topic6[50]="";
char topic7[50]="";
char topic8[50]="";
char topic9[50]="";
char topic10[50]="";
char topic11[50]="";
char topic12[50]="";

//------------------------Defining common portion of the topics------------------------//

char projectName[]="mushroom/";

//------------------------Subscribed topics--------------------------------------------//
char commonTopic0[50]="/user_input";
char commonTopic1[50]="/temp";
char commonTopic2[50]="/time";
char commonTopic3[50]="/chk-status";
char commonTopic4[50]="/deviceTime";
char commonTopic5[50]="/reset";
char commonTopic6[50]="/clearEEPROM";
char commonTopic7[50]="/checkEEPROM";

//------------------------Publishing topics--------------------------------------------//
char commonTopic8[50]="/sensor_data";
char commonTopic9[50]="/load_status";
char commonTopic10[50]="/ack";
char commonTopic11[50]="/online";
char commonTopic12[50]="/currentTime";



//----------------User did & uid spliting variables------------------------------------//

char Data[30];
const int numberOfPieces = 2;
String configData[numberOfPieces];
String did_uid="";
// Keep track of current position in time array
int counter = 0;

// Keep track of the last comma so we know where to start the substring
int lastIndex;

char did[24]={'0'};
char uid[24]={'0'};


//---------------Main Set up-----------------------------------------------------------//


void setup() {
  // put your setup code here, to run once:


  Serial.begin(115200);
  
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

  setup_wifi();
  client.setServer(mqtt_server,mqttPort);
  client.setCallback(callback);

}

void loop() {
  // put your main code here, to run repeatedly:

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

//  Serial.println(char(EEPROM.read(0)));

}

//------------------------WiFi Setup-------------------------------------//

void setup_wifi() {
    
    delay(100);
  // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}


//---------------------------While client not conncected---------------------------------//

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
      
      
     
     //once connected to MQTT broker, subscribe command if any
     //----------------------Subscribing to required topics-----------------------//

     


                  
                 readConfigDatafromEEPROM();

                  if(strlen(did)==0 && strlen(uid)==0)client.subscribe("mushroom/config");
                  
                  if(strlen(did)!=0 && strlen(uid)!=0){

                    client.unsubscribe("mushroom/config");
                    
                   
                    Serial.println("You already have did and uid assigned");
                    Serial.println("Unsubscribed from: mushroom/configData");
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
                      constructTopic();

                      
                  }
      
      
            
      } else {
      
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 6 seconds before retrying
      delay(5000);
    }
  }
}



//-----------------------Callback function-------------------------------------//


void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  


if(strcmp(topic, "mushroom/config") == 0){


  Serial.print("Message:");
  memset(Data, 0, sizeof(Data)/sizeof(char));//Resetting the array with all zeros
  Serial.print("Data array size:");
  Serial.println(sizeof(Data)/sizeof(char));
  Serial.print("Data before getting msg:");
  Serial.println(Data);

  
   


    for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    //input=input+payload[i];
    Data[i]=payload[i];
    }
      Serial.print("Data:");
      Serial.println(Data);
      did_uid=did_uid+Data;
      Serial.print("Device id,User id:");
      Serial.print(did_uid);
      Serial.println();
      // Loop through each character and check if it's a comma
      Serial.print("configData length:");
      Serial.println(did_uid.length());
      lastIndex =0;
      for (int i = 0;i < did_uid.length(); i++){  
        
        if (did_uid.substring(i, i+1) == ",") //input.substring: This method extracts the characters in a string 
                                            //between "start" and "end", not including "end" itself
        {
          // Grab the piece from the last index up to the current position and store it
          Serial.print("lastIndex:");
          Serial.println(lastIndex);
          configData[counter] = did_uid.substring(lastIndex, i);
          Serial.println(configData[counter]);
          // Update the last position and add 1, so it starts from the next character
          lastIndex = i + 1;
          // Increase the position in the array that we store into
          counter++;
        }

        // If we're at the end of the string (no more commas to stop us)
        Serial.print("last string start at:");
        Serial.println(i);
        if (i == did_uid.length()-1) {
          // Grab the last part of the string from the lastIndex to the end
          configData[counter] = did_uid.substring(lastIndex, i+1);
          Serial.println(configData[counter]);
        }}
         
      // Clear out string and counters to get ready for the next incoming string
      did_uid ="";
      counter = 0;
      lastIndex = 0;
      // Writing did and uid to EEPROM
      writeConfigDataToEEPROM();
          
            
      }
    
  
  }


 //------------------ Writing did and uid to EEPROM-----------------------------------------//

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


    did[i]=char(EEPROM.read(configDataReadAddress));
    configDataReadAddress++;
}

  Serial.print("did:");
  Serial.println(did);

  for(int j=0;j<10;++j){

       
      uid[j]=char(EEPROM.read(configDataReadAddress));
      configDataReadAddress++;
    
  }

  Serial.print("uid:");
  Serial.println(uid);
  }     

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

strcpy(topic6,projectName);
strcat(topic6,commonTopic6);
Serial.print("topic6:");
Serial.println(topic6);

strcpy(topic7,projectName);
strcat(topic7,commonTopic7);
Serial.print("topic7:");
Serial.println(topic7);

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

//ESP.restart();
} 

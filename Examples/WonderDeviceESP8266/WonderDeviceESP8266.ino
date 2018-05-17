#include <FS.h>                   //this needs to be first, or it all crashes and burns...
//will eventually store things here:
// https://github.com/jtmoderate876/WonderDeviceESP8266

#include <DoubleResetDetect.h>  //https://github.com/jenscski/DoubleResetDetect
#define DRD_TIMEOUT 2.0         // max seconds between resets for double reset
#define DRD_ADDRESS 0x00        // address of block of RTC user memory, may need to change it if it collides with another usage 
DoubleResetDetect drd(DRD_TIMEOUT, DRD_ADDRESS);

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "172.20.10.2";
char mqtt_port[6] = "1883";
char device_name[40] ;
char ultrarollovercountchar[7] ="1000";
char ultracountermaxdistancechar[4] = "30";
char topictemp[60];
/*
will automatically create:
 ./temperature/0/pv
 ./temperature/1/pv
 ./temperature/delta/pv
 ./distance/pv
 ./count/pv
*/
long  ultrarollovercount = 1000;
long  ultracountermaxdistance= 30; //30 cm or closer constitutes an object passing
char hiername_t0[60]        =  "temperature/0/pv";
char hiername_t1[60]        =  "temperature/1/pv";
char hiername_dt[60]        =  "temperature/delta/pv";//dt=t0-t1
char hiername_distance[60]  =  "distance/pv";// distance in cm
char hiername_count[60]     =  "count/pv";// 999 rollover counter (passing parts, people

String autoconf_apssid; // name of this device when used as accesspoint (during configuration)

//flag for saving data
bool shouldSaveConfig = false;

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

# 
long ultradistancelastMsg = 0;


WiFiClient espClient;
PubSubClient client(espClient);
long temperatureslastMsg = 0;
char msg[50];
int value = 0;

long now; //will contain milliseconds

//dallas onewire two temperature 
char temp0Fchar[6];
float temp0F = -999;
char temp1Fchar[6];
float temp1F = -999;
float difftemp0minus1F = -999;
char difftemp0minus1Fchar[6];
char prevtemp0Fchar[6];
char prevtemp1Fchar[6];
int ret = 0;
bool temperaturesupdaterequiredflag = false;
// Include the libraries we need

#include <OneWire.h>
#include <DallasTemperature.h>
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2
#define TEMPERATURE_PRECISION 12 // 9=0.9F, 10=0.45F, 11=0.225F, 12=0.1122F (.5C,.25C,.125c,.0625C)
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
//float prevtempF = -999;
// arrays to hold device addresses
DeviceAddress thermometer0, thermometer1;

//ultrasonic distance and rollover counter
#define ultratrigPin 5
#define ultraechoPin 4
int ultracounter = 0;
int ultracurrentState = 0;
int ultrapreviousState = 0;
long ultraduration, ultradistance;
long ultrapreviousdistance = 0;
long ultradistancediff = 0;
bool ultraupdaterequiredflag = false;
char ultracounterchar[6];
char ultradistancechar[6];


//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();//new line to clear noise/garbage

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  if (drd.detect()) {
    Serial.println("** Double reset boot - resetting wifimanager settings. Pay no attention to the next 3 lines (settings invalidated, THIS MAY..., and new line) from wifimanager:**");
    wifiManager.resetSettings();  //reset settings - for testing
    //do we want to do something with ??: 
    //clean FS, for testing
    SPIFFS.format();
  } else {
    Serial.println("** Normal boot **");
  }
  //clean FS, for testing
  //wifiManager.resetSettings();  //reset settings - for testing
  //SPIFFS.format();


  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
//          strcpy(blynk_token, json["blynk_token"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read



  autoconf_apssid = "ESP"+String(ESP.getChipId());//create a default device id of the form ESP plus the Chip ID
  strcpy(device_name, autoconf_apssid.c_str()); //also store it as devicename
  
  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  // WiFiManagerParameter custom_parameter( ID in wifi mgr, prompt, default value, max length);
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_device_name("topic", "mqtt base topic", device_name, 40);
  WiFiManagerParameter custom_counter_rollover("counterrollover", "counter rollover value", ultrarollovercountchar, 7);
  WiFiManagerParameter custom_counter_maxdist("countermaxdist", "max distance to count", ultracountermaxdistancechar, 4);
//  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);


  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
//  wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_device_name);
  wifiManager.addParameter(&custom_counter_rollover);
  wifiManager.addParameter(&custom_counter_maxdist);
//  wifiManager.addParameter(&custom_blynk_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
//  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
//  if (!wifiManager.autoConnect()) {
  if (!wifiManager.autoConnect(autoconf_apssid.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(device_name, custom_device_name.getValue());
  strcpy(ultrarollovercountchar, custom_counter_rollover.getValue());
  strcpy(ultracountermaxdistancechar, custom_counter_maxdist.getValue());
//  strcpy(blynk_token, custom_blynk_token.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["device_name"] = device_name;
    json["ultrarollovercountchar"] = ultrarollovercountchar;
    json["ultracountermaxdistancechar"] = ultracountermaxdistancechar;
//    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

//  Serial.println("local ip");
//  Serial.println(WiFi.localIP());

  ultrarollovercount = atoi(ultrarollovercountchar);
  ultracountermaxdistance = atoi(ultracountermaxdistancechar);
  Serial.print("ultrarollovercount = ");
  Serial.print(ultrarollovercount);
  Serial.print(" converted from ultrarollovercountchar = ");
  Serial.print(ultrarollovercountchar);
  Serial.print(" and ultracountermaxdistancechar = ");
  Serial.print(ultracountermaxdistancechar);
  Serial.print(" converted to = ");
  Serial.print(ultracountermaxdistance);
  Serial.println();



  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);

  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
//  setup_wifi();
  client.setServer(mqtt_server, atoi(mqtt_port));
//  client.setCallback(callback);

  // Start up the library (dallas)
  sensors.begin();

    // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  if (!sensors.getAddress(thermometer0, 0)) Serial.println("Unable to find address for Device thermometer0");
  if (!sensors.getAddress(thermometer1, 1)) Serial.println("Unable to find address for Device thermometer1");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(thermometer0);
  Serial.println();

  Serial.print("Device 1 Address: ");
  printAddress(thermometer1);
  Serial.println();

  // set the resolution to 9 bit per device
  sensors.setResolution(thermometer0, TEMPERATURE_PRECISION);
  sensors.setResolution(thermometer1, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(thermometer0), DEC);
  Serial.println();

  Serial.print("Device 1 Resolution: ");
  Serial.print(sensors.getResolution(thermometer1), DEC);
  Serial.println();

  //ultrasonic distance and rollover counter
  pinMode(ultratrigPin, OUTPUT);
  pinMode(ultraechoPin, INPUT);

   //add devicename and "/" to front of all the topics
   char tempchar[60];
  sprintf(tempchar, "%s/%s", device_name, hiername_t0);
  strcpy(hiername_t0, tempchar);
  sprintf(tempchar, "%s/%s", device_name, hiername_t1);
  strcpy(hiername_t1, tempchar);
  sprintf(tempchar, "%s/%s", device_name, hiername_dt);
  strcpy(hiername_dt, tempchar);
  sprintf(tempchar, "%s/%s", device_name, hiername_distance);  // distance in cm
  strcpy(hiername_distance, tempchar);
  sprintf(tempchar, "%s/%s", device_name, hiername_count);        // 999 rollover counter (passing parts, people...)
  strcpy(hiername_count, tempchar);
  Serial.print("For example, hiername_count=" );
  Serial.println(hiername_count);
}

//mqtt
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// more dallas functions
// dallas function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// dallas function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// dallas function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

// dallas main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}




void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  now = millis();

//  Serial.print(now);   Serial.print(" ");  Serial.print(temperaturesupdaterequiredflag);  Serial.print(" ");  Serial.print(temperatureslastMsg);  Serial.print(" ");
  
  temperaturesupdaterequiredflag = false;
  if (now - temperatureslastMsg > 2000) {
    temperatureslastMsg = now;
  
    // Fetch temperatures from Dallas sensors
    // call sensors.requestTemperatures() to issue a global temperature 
    // request to all devices on the bus
    //Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
   
    // After we got the temperatures, we can print them here.
    // We use the function ByIndex, and as an example get the temperature from the first sensor only.
    temp0F = sensors.getTempFByIndex(0);
    temp0F = roundf(temp0F*10)/10; // 0.1 deg F precision
    ret = snprintf(temp0Fchar,sizeof temp0Fchar, "%.1f", temp0F);
    client.publish(hiername_t0, temp0Fchar);

    temp1F = sensors.getTempFByIndex(1);
    temp1F = roundf(temp1F*10)/10; // 0.1 deg F precision
    ret = snprintf(temp1Fchar,sizeof temp1Fchar, "%.1f", temp1F);
    client.publish(hiername_t1, temp1Fchar);

    difftemp0minus1F = temp0F - temp1F;
//    difftemp0minus1F = roundf(difftemp0minus1F *10)/10; // 0.1 deg F precision
    ret = snprintf(difftemp0minus1Fchar,sizeof difftemp0minus1Fchar, "%.1f", difftemp0minus1F);
    client.publish(hiername_dt, difftemp0minus1Fchar);
    
    // for serial print only print if there is change
    if (prevtemp0Fchar!=temp0Fchar) {
      strcpy(prevtemp0Fchar, temp0Fchar);
      temperaturesupdaterequiredflag = true;
    }
    if (prevtemp1Fchar!=temp1Fchar) {
      strcpy(prevtemp1Fchar, temp1Fchar);
      temperaturesupdaterequiredflag = true;
    }
  }

  //ultrasonic
  ultraupdaterequiredflag = false;
  if (now - ultradistancelastMsg > 100) {
//    Serial.print(" inside ultra with time of ");
//    Serial.print(ultradistancelastMsg );
    ultradistancelastMsg = now;
//    ultracountlastMsg = 0;

    digitalWrite(ultratrigPin, LOW); 
    delayMicroseconds(2); 
    digitalWrite(ultratrigPin, HIGH);
    delayMicroseconds(10); 
    digitalWrite(ultratrigPin, LOW);
    ultraduration = pulseIn(ultraechoPin, HIGH);
    ultradistance = (ultraduration/2) / 29.1;
  
    ultradistancediff = ultrapreviousdistance - ultradistance;
    if (abs(ultradistancediff)>1) {
      ultrapreviousdistance = ultradistance;
      ultraupdaterequiredflag = true;
      sprintf(ultradistancechar , "%d", ultradistance);
//      sprintf(ultradistancechar , "%04d", ultradistance);
      client.publish(hiername_distance , ultradistancechar);
    }

    if (ultradistance <= ultracountermaxdistance){
      ultracurrentState = 1;
    }
    else {
      ultracurrentState = 0;
    }
    if(ultracurrentState != ultrapreviousState){
      if(ultracurrentState == 1){
        ultracounter = ultracounter + 1;
        if (ultracounter > ultrarollovercount ) {
          ultracounter = ultracounter - ultrarollovercount;      
        }
        ultraupdaterequiredflag = true;
        ultracounterchar;
        sprintf(ultracounterchar, "%d", ultracounter);
        client.publish(hiername_count , ultracounterchar);
      }
    }
    ultrapreviousState = ultracurrentState;
  }


  //Serial.print if needed
  if ( ultraupdaterequiredflag | temperaturesupdaterequiredflag) {
    Serial.print("Counter: ");  
//    Serial.print(ultracounter); 
    Serial.print(  ultracounter/100 );
    Serial.print((ultracounter/10)%10);
    Serial.print(ultracounter%10);
    Serial.print(" Distance: ");
    Serial.print(ultradistance );
    Serial.print(" cm");  

    Serial.print("   Temps in deg F: Temp0: ");
    Serial.print(temp0Fchar);
    Serial.print(" and Temp1: ");
    Serial.print(temp1Fchar);
    Serial.print(" and t0-t1: ");
    Serial.print(difftemp0minus1Fchar);

    Serial.println();
  }


}

/*#################################An example to connect thingcontro.io MQTT over TLS1.2###############################
*/
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
//  #include "Adafruit_MCP23008.h"    // i2c lib to control relay shield

// Modbus
//#include <ModbusMaster.h>
//#include "REG_CONFIG.h"
//#include <HardwareSerial.h>
//#include <BME280I2C.h>
#include "Adafruit_SHT31.h"
Adafruit_SHT31 sht31 = Adafruit_SHT31();

#include <ESPUI.h>
#include <ESPmDNS.h>
//#include <SPI.h>

#include "http_ota.h"
//  HardwareSerial modbus(2);
//  BME280I2C bme;

void enterDetailsCallback(Control *sender, int type);
void sendAttribute();
void processAtt(char jsonAtt[]);
void reconnectMqtt();
void heartBeat();
void Task1code(void *pvParameters);

#define WIFI_AP ""
#define WIFI_PASSWORD ""
#define HOSTNAME "SmartEnv:4c:75:25:56:a1:84"
#define FORCE_USE_HOTSPOT 0

#define WDTPin 4
#define trigWDTPin 33
#define ledHeartPIN 0

String deviceToken = "4c:75:25:56:a1:84";
char thingsboardServer[] = "tb.thingcontrol.io";

String json = "";

//  ModbusMaster node;

//static const char *fingerprint PROGMEM = "69 E5 FE 17 2A 13 9C 7C 98 94 CA E0 B0 A6 CB 68 66 6C CB 77"; // need to update every 3 months
int periodSendTelemetry = 60;  //the value is a number of seconds

//UI handles
uint16_t wifi_ssid_text, wifi_pass_text;
uint16_t nameLabel, idLabel, grouplabel, grouplabel2, mainSwitcher, mainSlider, mainText, settingZNumber, resultButton, mainTime, downloadButton, selectDownload, logStatus;
//  uint16_t styleButton, styleLabel, styleSwitcher, styleSlider, styleButton2, styleLabel2, styleSlider2;
uint16_t tempText, humText, humText2, saveConfigButton, interval ,emailText1, cuationlabel, lineText;
uint16_t shtLog, wifiLog, teleLog;

uint16_t graph;
volatile bool updates = false;
String email1 = "";
String lineID = "";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

WiFiManager wifiManager;

// Basic toggle test for i/o expansion. flips pin #0 of a MCP23008 i2c
// pin expander up and down. Public domain

// Connect pin #1 of the expander to Analog 5 (i2c clock)
// Connect pin #2 of the expander to Analog 4 (i2c data)
// Connect pin #3, 4 and 5 of the expander to ground (address selection)
// Connect pin #6 and 18 of the expander to 5V (power and reset disable)
// Connect pin #9 of the expander to ground (common ground)

// Output #0 is on pin 10 so connect an LED or whatever from that to ground

//  Adafruit_MCP23008 mcp;

int status = WL_IDLE_STATUS;
String downlink = "";
char *bString;
int PORT = 1883;

// Modbus
float temp(NAN), hum(NAN), pres(NAN);
int TempOffset = 0;
int HumOffset1 = 0;
String shtStatus = "";
String mqttStatus = "";

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

// This is the main function which builds our GUI
void setUpUI() {

  //Turn off verbose  ging
  ESPUI.setVerbosity(Verbosity::Quiet);

  //Make sliders continually report their position as they are being dragged.
  ESPUI.sliderContinuous = true;

  //This GUI is going to be a tabbed GUI, so we are adding most controls using ESPUI.addControl
  //which allows us to set a parent control. If we didn't need tabs we could use the simpler add
  //functions like:
  //    ESPUI.button()
  //    ESPUI.label()
  
  /*
     Tab: Basic Controls
     This tab contains all the basic ESPUI controls, and shows how to read and update them at runtime.
    -----------------------------------------------------------------------------------------------------------*/
  auto maintab = ESPUI.addControl(Tab, "", "Home");
  nameLabel = ESPUI.addControl(Label, "Device Name", "TempMon 1", Emerald, maintab);
  idLabel = ESPUI.addControl(Label, "Device ID", String(deviceToken), Emerald, maintab);

  auto settingTab = ESPUI.addControl(Tab, "", "Setting");
  ESPUI.addControl(Separator, "", "", None, settingTab);
  cuationlabel = ESPUI.addControl(Label, "Cuation", "Offset will be divided by 100 after saving.", Emerald, settingTab);
  ESPUI.addControl(Separator, "Offset Configuration", "", None, settingTab);
  tempText = ESPUI.addControl(Number, "Temperature", String(TempOffset), Emerald, settingTab, enterDetailsCallback);
  humText = ESPUI.addControl(Number, "Humidity 1", String(HumOffset1), Emerald, settingTab, enterDetailsCallback);
  
  ESPUI.addControl(Separator, "Interval Configuration", "", None, settingTab);
  interval = ESPUI.addControl(Number, "Interval (second)", String(periodSendTelemetry), Emerald, settingTab, enterDetailsCallback);
  
  /*
  ESPUI.addControl(Separator, "", "", None, settingTab);
  ESPUI.addControl(Button, "Save", "SAVE", Peterriver, settingTab, enterDetailsCallback);
  */
  
  ESPUI.addControl(Separator, "Email Registry", "", None, settingTab);
  /**/
  emailText1 = ESPUI.addControl(Text, "Email User 1", "", Emerald, settingTab, enterDetailsCallback);
  lineText = ESPUI.addControl(Text, "Line ID", "", Emerald, settingTab, enterDetailsCallback);
  /*
  emailText2 = ESPUI.addControl(Text, "Email User 2", email.email2, Emerald, settingTab, enterDetailsCallback);
  emailText3 = ESPUI.addControl(Text, "Email User 3", email.email3, Emerald, settingTab, enterDetailsCallback);
  emailText4 = ESPUI.addControl(Text, "Email User 4", email.email4, Emerald, settingTab, enterDetailsCallback);
  emailText5 = ESPUI.addControl(Text, "Email User 5", email.email5, Emerald, settingTab, enterDetailsCallback);
  /**/

  ESPUI.addControl(Separator, "", "", None, settingTab);
  //  ESPUI.addControl(Button, "Refresh", "Refresh", Peterriver, settingTab, enterDetailsCallback);
  ESPUI.addControl(Button, "Save", "SAVE", Peterriver, settingTab, enterDetailsCallback);
  
  auto eventTab = ESPUI.addControl(Tab, "", "Event Log");
  ESPUI.addControl(Separator, "Error Log", "", None, eventTab);
  teleLog = ESPUI.addControl(Label, "Server Connection Status", String(mqttStatus), Alizarin, eventTab, enterDetailsCallback);
  shtLog = ESPUI.addControl(Label, "Sensor Connection Status", String(shtStatus), Alizarin, eventTab, enterDetailsCallback); 

  //Finally, start up the UI.
  //This should only be called once we are connected to WiFi.
  ESPUI.begin(HOSTNAME);
  
  }

void enterDetailsCallback(Control *sender, int type) {
  Serial.println(sender->value);
  ESPUI.updateControl(sender);

  if(type == B_UP) {
      Serial.println("Saving Offset to EPROM...");

    // Fetch controls
    Control* TempOffset_ = ESPUI.getControl(tempText);
    Control* HumOffset1_ = ESPUI.getControl(humText);
    Control* periodSendTelemetry_ = ESPUI.getControl(interval);
    Control* email1_ = ESPUI.getControl(emailText1);
    Control* lineID_ = ESPUI.getControl(lineText);
    
    // Store control values
    TempOffset = TempOffset_->value.toFloat();
    HumOffset1 = HumOffset1_->value.toFloat();
    periodSendTelemetry = periodSendTelemetry_->value.toFloat();
    email1 = email1_->value;
    lineID = lineID_->value;
    char data1[50];
    char data2[50];
    email1.toCharArray(data1, 50);  // Convert String to char array
    lineID.toCharArray(data2, 50);
    
    // Print to Serial
    Serial.println("put TempOffset: " + String(TempOffset));
    Serial.println("put HumOffset1: " + String(HumOffset1));
    Serial.println("put periodSendTelemetry: " + String(periodSendTelemetry));
    Serial.println("put email1: " + String(email1));


    // Write to EEPROM
    EEPROM.begin(150); // Ensure enough size for data
    int addr = 0;

    EEPROM.put(addr, TempOffset);
    addr += sizeof(TempOffset);
    EEPROM.put(addr, HumOffset1);
    addr += sizeof(HumOffset1);
    EEPROM.put(addr, periodSendTelemetry);
    //  addr += sizeof(periodSendTelemetry);
    addr = 30;
    for (int len = 0; len < email1.length(); len++) {
      EEPROM.write(addr + len, data1[len]);  // Write each character
    }
    EEPROM.write(addr + email1.length(), '\0');  // Add null terminator at the end
    addr = 50;
    for (int len = 0; len < lineID.length(); len++) {
      EEPROM.write(addr + len, data2[len]);  // Write each character
    }
    EEPROM.write(addr + lineID.length(), '\0');  // Add null terminator at the end
    EEPROM.commit();
    //  addr += (email1.length() + 1); For the future project
    //Dubug Address
    /*
    for (int i = 0; i < 50; i++) { // Reading the first 10 bytes
    Serial.print("Address ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i)); // Likely will print 255
  }
  */
    EEPROM.end();
    sendAttribute(); // Assuming this function is required to send attributes
    Serial.println("debugendeprom");
  }
}

void readEEPROM() {
  Serial.println("Reading credentials from EEPROM...");
  EEPROM.begin(150); // Ensure enough size for data
  
  //Dubug Address
    /*
    for (int i = 0; i < 50; i++) { // Reading the first 10 bytes
    Serial.print("Address ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i)); // Likely will print 255
  }
  */
  
  int addr = 0;
    EEPROM.get(addr, TempOffset);
    addr += sizeof(TempOffset);
    EEPROM.get(addr, HumOffset1);
    addr += sizeof(HumOffset1);
    EEPROM.get(addr, periodSendTelemetry);
    //  addr += sizeof(periodSendTelemetry);
    addr = 30;
    for (int len = 0; len < 50; len++){
      char data1 = EEPROM.read(addr + len);
      if(data1 == '\0' || data1 == 255) break;
      email1 += data1;
    }
    //  addr += sizeof(email1);
    addr = 50;
    for (int len = 0; len < 50; len++){
      char data2 = EEPROM.read(addr + len);
      if(data2 == '\0' || data2 == 255) break;
      lineID += data2;
    }
    EEPROM.end();
  // Print to Serial
  Serial.println("get TempOffset: " + String(TempOffset));
  Serial.println("get HumOffset1: " + String(HumOffset1));
  Serial.println("get periodSendTelemetry: " + String(periodSendTelemetry));
  Serial.println("get outputEmail1: " + String(email1));

  ESPUI.updateNumber(tempText, TempOffset);
  ESPUI.updateNumber(humText, HumOffset1);
  ESPUI.updateNumber(interval, periodSendTelemetry);
  ESPUI.updateText(emailText1, String(email1));
  ESPUI.updateText(lineText, String(lineID));
}

void _initsht() {
  if (!sht31.begin(0x44)) {
    shtStatus = "SHT31 sensor not found";
    ESPUI.updateLabel(shtLog, String(shtStatus));
  } else {
    shtStatus = "SHT31 sensor initialized";
    ESPUI.updateLabel(shtLog, String(shtStatus));
  }
}

void setup() {
  Project = "TempMon";
  FirmwareVer = "1.5";
  Serial.begin(115200);
  Wire.begin();
  _initsht();
  Serial.println(F("Starting... SHT20 TEMP/HUM_RS485 Monitor"));
  wifiManager.setAPCallback(configModeCallback);
  if (!wifiManager.autoConnect("SmartEnv:4c:75:25:56:a1:84")) {
    Serial.println("failed to connect and hit timeout");
    delay(1000);
  }
  //  getMac();
  client.setServer(thingsboardServer, PORT);
  reconnectMqtt(); // Initial MQTT connect
  Serial.println("Start..");
  MDNS.begin(HOSTNAME);
  WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(HOSTNAME);
  setUpUI();
  delay(200);
  readEEPROM();
  Serial.println("debugendSetUP");
}

void loop() {  
   const unsigned long time2send = periodSendTelemetry * 1000;
  // Check telemetry timing
  if (millis() % time2send == 0) {
    
    json = "";
    Serial.println("Sending telemetry...");
    temp = sht31.readTemperature() + (TempOffset / 100);
    hum = sht31.readHumidity() + (HumOffset1 / 100);
    json.concat("{\"temp\":");
    //  json.concat("33");
    json.concat(String(temp));
    json.concat(",\"hum\":");
    //  json.concat("33");
    json.concat(String(hum));
    json.concat("}");
    Serial.println(json);
    // Length (with one extra character for the null terminator)
    int str_len = json.length() + 1;
    // Prepare the character array (the buffer)
    char char_array[str_len];
    // Copy it over
    json.toCharArray(char_array, str_len);
    client.publish( "v1/devices/me/telemetry", char_array);
    //sendtelemetry();  // Function to send the telemetry data
    Serial.println("Telemetry sent");
  }
  
  if (millis() % 10000 == 0) {
    OTA_git_CALL();
    heartBeat();
    status = WiFi.status();
    if (status == WL_CONNECTED) {
      if (!client.connected()) {
        Serial.println("Client disconnected, attempting to reconnect...");
        reconnectMqtt();
      }
      client.loop(); // Process MQTT messages
    } else {
      Serial.println("WiFi disconnected");
    }
  }
}

void getMac()
{
  Serial.println("OK");
  Serial.print("+deviceToken: ");
  Serial.println(WiFi.macAddress());
  deviceToken = WiFi.macAddress();
  ESPUI.updateLabel(idLabel, String(deviceToken));
}

void reconnectMqtt()
{
  if ( client.connect("Thingcontrol_AT", deviceToken.c_str(), NULL) )
  {
    mqttStatus = "Succeed to Connect Server!";
    ESPUI.updateLabel(teleLog, String(mqttStatus));
    Serial.println(mqttStatus);
    client.subscribe("v1/devices/me/rpc/request/+");
  }else{
    mqttStatus = "Failed to Connect Server!";
    Serial.println(mqttStatus);
    ESPUI.updateLabel(teleLog, String(mqttStatus));
  }
}

void sendAttribute(){
  String json = "";
  json.concat("{\"email1\":\"");
  json.concat(String(email1));
  json.concat("\",\"lineID\":\"");
  json.concat(String(lineID));
  json.concat("\"}");
  Serial.println(json);
  // Length (with one extra character for the null terminator)
  int str_len = json.length() + 1;
  // Prepare the character array (the buffer)
  char char_array[str_len];
  // Copy it over
  json.toCharArray(char_array, str_len);
  processAtt(char_array);
}

void processAtt(char jsonAtt[])
{
  char *aString = jsonAtt;
  Serial.println("OK");
  Serial.print(F("+:topic v1/devices/me/attributes , "));
  Serial.println(aString);
  client.publish( "v1/devices/me/attributes", aString);
}

void heartBeat()
{
  //   Sink current to drain charge from watchdog circuit
  pinMode(trigWDTPin, OUTPUT);
  digitalWrite(trigWDTPin, LOW);

  // Led monitor for Heartbeat
  digitalWrite(ledHeartPIN, LOW);
  delay(100);
  digitalWrite(ledHeartPIN, HIGH);

  // Return to high-Z
  pinMode(trigWDTPin, INPUT);

  Serial.println("Heartbeat");
  // SerialBT.println("Heartbeat");
}
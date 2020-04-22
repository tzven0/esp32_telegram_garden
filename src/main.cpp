#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "BMP280.h"
#include "Wire.h"
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>

#define APP_WIFI_SSID CONFIG_WIFI_SSID
#define APP_WIFI_PWD CONFIG_WIFI_PASSWORD
#define APP_BOT_TOKEN CONFIG_BOT_TOKEN
#define SENDER_ID_COUNT 3
#define APP_CHAT_ID CONFIG_CHAT_ID

WiFiClientSecure client;
UniversalTelegramBot bot(APP_BOT_TOKEN, client);
uint32_t lastCheckTime = 0;
String validSenderIds[SENDER_ID_COUNT] = {"012345678", "123456789", APP_CHAT_ID};
const String KLIMA = "Klima";
const String START = "/start";

#define P0 1013.25
BMP280 bmp;
double T_bmp,P_bmp,A_bmp;

#define DHTPIN 4     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 11
double T_dht,H_dht;

#define P0 1013.25

#define rainSensePin 5
#define rainPowerPin 18
bool is_raining;

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

void init_bmp() {
  if(!bmp.begin()){
    Serial.println("BMP init failed!");
    while(1);
  }
  else Serial.println("BMP init success!");
  
  bmp.setOversampling(4);
}

void init_dht() {
  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

int scan_wifi() {
  // WiFi.scanNetworks will return the number of networks found
  int found_ap = 0;
  int n = WiFi.scanNetworks();
  if (n == 0) {
      Serial.println("no networks found");
  } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
          // Print SSID and RSSI for each network found
          Serial.print(i + 1);
          Serial.print(": ");
          Serial.print(WiFi.SSID(i));
          Serial.print(" (");
          Serial.print(WiFi.RSSI(i));
          Serial.print(")");
          Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
          if(WiFi.SSID(i) == APP_WIFI_SSID)
            found_ap = 1;
          delay(10);
      }
  }
  Serial.println("");
  return found_ap;
}

void connect_to_ap() {
  Serial.print("Connecting to ");
  Serial.println(APP_WIFI_SSID);

  WiFi.begin(APP_WIFI_SSID, APP_WIFI_PWD);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  pinMode(rainPowerPin, OUTPUT);
  digitalWrite(rainPowerPin, LOW);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if(scan_wifi() == 1) {
    Serial.println(F("Found AP!"));
    connect_to_ap();
  } else {
    Serial.println(F("Can not find AP! Not connected!"));
  }

  init_bmp();

  init_dht();

  delay(delayMS);

  Serial.println("Setup done");
}

//void get_rain_reading() {
void get_rain_reading(bool &is_raining) {
  digitalWrite(rainPowerPin, HIGH);	// Turn the sensor ON
	delay(100);							// Allow power to settle
	int val = digitalRead(rainSensePin);	// Read the sensor output
	digitalWrite(rainPowerPin, LOW);		// Turn the sensor OFF
  Serial.print(F("[Rain] Raining = \t"));
  Serial.println((val == 0)?"Yes":"No");
  (val == 0)?is_raining = true:is_raining = false;
}

//void get_bmp_data() {
void get_bmp_data(double &T_bmp, double &P_bmp, double &A_bmp) {
  //double T_bmp,P_bmp;
  char result = bmp.startMeasurment();
 
  if(result!=0){
    delay(result);
    result = bmp.getTemperatureAndPressure(T_bmp,P_bmp);
    
      if(result!=0)
      {
        //double A_bmp = bmp.altitude(P_bmp,P0);
        A_bmp = bmp.altitude(P_bmp,P0);
        
        Serial.print(F("[BMP] T = \t"));
        Serial.print(T_bmp,2);
        Serial.print(F(" °C\t"));
        Serial.print(F("Pressure = \t"));
        Serial.print(P_bmp,2);
        Serial.print(F(" mBar\t"));
        Serial.print(F("Altitude = \t"));
        Serial.print(A_bmp,2);
        Serial.println(F(" m"));
       
      }
      else {
        Serial.println("[BMP] Error reading sensor data.");
        T_bmp = 99;
        P_bmp = 99;
        A_bmp = 99;
      }
  }
  else {
    Serial.println("[BMP] Measurement Error.");
  }
}

//void get_dht_data() {
void get_dht_data(double &T_dht, double &H_dht) {
  // Get temperature event and print its value.
  //double T_dht,H_dht;
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("[DHT] Error reading temperature!"));
    T_dht = 99;
  }
  else {
    T_dht = event.temperature;
    Serial.print(F("[DHT] T = \t"));
    Serial.print(T_dht);
    Serial.print(F(" °C\t"));
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
    H_dht = 99;
  }
  else {
    H_dht = event.relative_humidity;
    Serial.print(F("Humidity = \t"));
    Serial.print(H_dht);
    Serial.println(F(" %"));
  }
  // TODO: hole T und H und checke dann mit isnan()
  Serial.println("");
}

boolean validateSender(String senderId)
{
  for(int i=0; i<SENDER_ID_COUNT; i++)
  {
    if(senderId == validSenderIds[i])
    {
      return true;
    }
  }
  return false;
}

String getCommands()
{
  String message = "Verfügbare Kommandos sind:\n\n";
  message += "<b>" + KLIMA + "<b>: um das Wetter zu überprüfen\n";
  return message;
}

String getSensorMessage()
{
  get_dht_data(T_dht,H_dht);
  get_bmp_data(T_bmp,P_bmp,A_bmp);
  get_rain_reading(is_raining);
  String message = "";
  message += "Sensor 1: Temperatur " + String(T_dht)+ " °C, ";
  message += "Luftfeuchtigkeit " + String(H_dht) + " %\n";
  message += "Sensor 2: Temperatur " + String(T_bmp)+ " °C, ";
  message += "Luftdruck " + String(P_bmp) + " mBar, ";
  message += "Altitude " + String(A_bmp) + " Meter\n";
  return message;
}

void handleSensors(String chatId)
{
  bot.sendMessage(chatId, getSensorMessage(), "");
}

void handleStart(String chatId, String fromName)
{
  String message = "<b>Hello" + fromName + ".</b>\n";

  message += getCommands();

  bot.sendMessage(chatId, message, "HTML");
}

void handleNotFound(String chatId)
{
  String message = "Befehl nicht gefunden.\n";
  message += getCommands();
  bot.sendMessage(chatId, message, "HTML");
}

void handleNewMessages(int numNewMessages)
{
  for (int i=0; i<numNewMessages; i++)
  {
    String chatId = String(bot.messages[i].chat_id);
    String senderId = String(bot.messages[i].from_id);
     
    Serial.println("senderId: " + senderId);

    boolean validSender = validateSender(senderId);
 
    if(!validSender)
    {
      bot.sendMessage(chatId, "Sorry, no permission!", "HTML");
      continue;
    }
     
    String text = bot.messages[i].text; //texto que chegou
 
    if (text.equalsIgnoreCase(START))
    {
      handleStart(chatId, bot.messages[i].from_name); //mostra as opções
    }
    else if(text.equalsIgnoreCase(KLIMA))
    {
      handleSensors(chatId); //envia mensagem com a temperatura e umidade
    }
    else
    {
      handleNotFound(chatId); //mostra mensagem que a opção não é válida e mostra as opções
    }
  }//for
}

void loop() {

  get_bmp_data(T_bmp,P_bmp,A_bmp);
  
  get_dht_data(T_dht,H_dht);
  
  get_rain_reading(is_raining);

  uint32_t now = millis();
  if (now - lastCheckTime > 3000) 
  {
    Serial.println("IN MESSAGE HANDLE");
    lastCheckTime = now;
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    handleNewMessages(numNewMessages);
  }

  // Wait a bit before scanning again
  delay(delayMS);
}
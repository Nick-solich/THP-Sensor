//USED LIBARY 
#include "DHT.h"
#include <PMserial.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <time.h>

//UTC OFFSET
#define UTC_OFFSET 25200

//DHT DEF
#define DHTPIN 2
#define DHTTYPE DHT11   // DHT 11
DHT dht(DHTPIN, DHTTYPE);

//FIREBASE WIFI DEF
#define WIFI_SSID "PROJ"
#define WIFI_PASSWORD "test1234"
#define API_KEY "AIzaSyAmhyLiWn4nLHv0BxCtb5NldybASBgIceQ"
#define DATABASE_URL "https://esp8266-thpsensor-default-rtdb.asia-southeast1.firebasedatabase.app/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define USER_EMAIL "mariomater2001@gmail.com"
#define USER_PASSWORD "test1234"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;

//SOFTWARE SERIAL
SoftwareSerial StmSerial(13,15); //RX, TX
uint8_t buffer[1];

//PM SERIAL
constexpr auto PMS_RX = 5;
constexpr auto PMS_TX = 4;
SerialPM pms(PMS3003, PMS_RX, PMS_TX);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET);

void setup() {
  Serial.begin(9600);
  StmSerial.begin(9600);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Or use legacy authenticate method
  // config.database_url = DATABASE_URL;
  // config.signer.tokens.legacy_token = "<database secret>";

  // To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino

  //////////////////////////////////////////////////////////////////////////////////////////////
  // Please make sure the device free Heap is not lower than 80 k for ESP32 and 10 k for ESP8266,
  // otherwise the SSL connection will fail.
  //////////////////////////////////////////////////////////////////////////////////////////////

  Firebase.begin(&config, &auth);

  // Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);

  Firebase.setDoubleDigits(5);
  timeClient.begin();
  dht.begin();
  pms.init();
}

void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  timeClient.update();

  unsigned long epochTime = timeClient.getEpochTime();

  String TimeNow = String(epochTime);
  
  //Read pms value
  pms.read();

  // Read Humidity as Percent
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);
  
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
     Serial.printf("FAILED\r\n");
    return;
  }
  
  Serial.printf("PM1.0 %2d, PM2.5 %2d, PM10 %2d [ug/m3]\r\n",
                  pms.pm01, pms.pm25, pms.pm10);
  Serial.printf("Humidity: %.2f %% Temperature: %.2f°C  %.2f°F\r\n",
                  h, t, f);

  //Send data through StmSerial
  buffer[0] = pms.pm25;
  StmSerial.write(buffer, sizeof(buffer));

  String PATH;
  
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0) && epochTime > 1641038400)
  {
    sendDataPrevMillis = millis();
    PATH = "UserData/readings/" + TimeNow + "/" + "Humidity";
    Firebase.setFloat(fbdo, PATH,h);
    PATH = "UserData/readings/" + TimeNow + "/" + "TemperatureToCelsius";
    Firebase.setFloat(fbdo, PATH,t);
    PATH = "UserData/readings/" + TimeNow + "/" + "TemperatureToFahrenheit";
    Firebase.setFloat(fbdo, PATH,f);
    PATH = "UserData/readings/" + TimeNow + "/" + "PM01";
    Firebase.setInt(fbdo, PATH,pms.pm01);
    PATH = "UserData/readings/" + TimeNow + "/" + "PM25";
    Firebase.setInt(fbdo, PATH,pms.pm25);
    PATH = "UserData/readings/" + TimeNow + "/" + "PM10";
    Firebase.setInt(fbdo, PATH,pms.pm10);
    PATH = "UserData/readings/" + TimeNow + "/" + "TimeStamp";
    Firebase.setString(fbdo, PATH,TimeNow);
  }

}

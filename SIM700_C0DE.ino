#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
//#include <ESP8266WiFi.h>                                                    // esp8266 library
//#include <FirebaseArduino.h>                                                // firebase library
//#include <FirebaseESP8266.h>

#include <Adafruit_MLX90614.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
/* 1. Define the WiFi credentials */
#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

/* 2. Define the API Key */
#define API_KEY "API_KEY"

/* 3. Define the RTDB URL */
#define DATABASE_URL "URL" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
#define  Gas_pin  12 // MQ-138 Sensor
int pirsensor = 14; // D7
Adafruit_MLX90614 mlx = Adafruit_MLX90614(); //Temperature sensor
FirebaseData firebaseData;
FirebaseJson json;
//miss code end

// Arduino Code
#include "Adafruit_FONA.h"
HardwareSerial SerialAT(1);

//const int RELAY_PIN = 15;
const int FONA_RST = 4;

char replybuffer[255];
uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
String smsString = "";
char fonaNotificationBuffer[64];          //for notifications from the FONA
char smsBuffer[250];

HardwareSerial *fonaSerial = &SerialAT;

Adafruit_FONA_3G fona = Adafruit_FONA_3G(FONA_RST);

void setup() 
{ Serial.begin(115200);
  mlx.begin();
  pinMode(pirsensor, INPUT); // declare sensor as input
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
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  //Or use legacy authenticate method
  //config.database_url = DATABASE_URL;
  //config.signer.tokens.legacy_token = "<database secret>";

  Firebase.begin(&config, &auth);

  //Comment or pass false value when WiFi reconnection will control by your code or third party library
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);
  //miss code end

  
  //pinMode(RELAY_PIN, OUTPUT);
  //digitalWrite(RELAY_PIN, LOW);
  
  Serial.begin(115200);
  Serial.println(F("FONA SMS caller ID test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

//  fonaSerial ->begin(115200,SERIAL_8N1,16,17, false);
  if (! fona.begin(*fonaSerial))
  {
    Serial.println(F("Couldn't find FONA"));
    while(1);
  }
  Serial.println(F("FONA is OK"));
  
  fonaSerial->print("AT+CNMI=2,1\r\n");  //set up the FONA to send a +CMTI notification when an SMS is received
  Serial.println("FONA Ready");
}

void loop() 
{ long state = digitalRead(pirsensor);
  float Temp = mlx.readObjectTempC();
  int Diabetic_value = analogRead(Gas_pin);
  float val;
  Diabetic_value = Diabetic_value / 10;
  val = Diabetic_value;
  Firebase.RTDB.setString(&fbdo, "/Data/Temperature", String(Temp));
  Firebase.RTDB.setString(&fbdo, "/Data/Glucose_level", String(val));
  Firebase.RTDB.setString(&fbdo, "/Data/Motion_Sensor", String(state));
  Serial.println("Body temperature =");
  Serial.println(Temp);
  Serial.print("Raw data : ");
  Serial.print(val);
  Serial.print('\n');
 /* if (state == HIGH) {
    //digitalWrite (Status, HIGH);
    Serial.println("Motion detected!");
  }
  else {
    //digitalWrite (Status, LOW);
    Serial.println("Motion absent!");
  }*/
  
  //miss code end
  char* bufPtr = fonaNotificationBuffer;    //handy buffer pointer
  
  if (fona.available())      //any data available from the FONA?
  {
    int slot = 0;            //this will be the slot number of the SMS
    int charCount = 0;
    //Read the notification into fonaInBuffer
    do
    {
      *bufPtr = fona.read();
      Serial.write(*bufPtr);
      delay(1);
    } while ((*bufPtr++ != '\n') && (fona.available()) && (++charCount < (sizeof(fonaNotificationBuffer)-1)));

    //Add a terminal NULL to the notification string
    *bufPtr = 0;

    //Scan the notification string for an SMS received notification.
    //  If it's an SMS message, we'll get the slot number in 'slot'
    if (1 == sscanf(fonaNotificationBuffer, "+CMTI: " FONA_PREF_SMS_STORAGE ",%d", &slot)) 
    {
      Serial.print("slot: "); Serial.println(slot);

      char callerIDbuffer[32];  //we'll store the SMS sender number in here

      // Retrieve SMS sender address/phone number.
      if (! fona.getSMSSender(slot, callerIDbuffer, 31)) 
      {
        Serial.println("Didn't find SMS message in slot!");
      }
      Serial.print(F("FROM: ")); Serial.println(callerIDbuffer);

      // Retrieve SMS value.
      uint16_t smslen;
      if (fona.readSMS(slot, smsBuffer, 250, &smslen)) // pass in buffer and max len!
      { 
        smsString = String(smsBuffer);
        Serial.println(smsString);
      }

      if (state == HIGH) {
        //digitalWrite (Status, HIGH);
        Serial.println("Motion detected!");
        fona.sendSMS(callerIDbuffer,"Motion is detected" );
  
      }
      else{
        //digitalWrite (Status, LOW);
        Serial.println("Motion absent!");  
      }
      if (Temp>=37){
        
        Serial.println("Tempreature is too high");
        delay(100);
        fona.sendSMS(callerIDbuffer,"Tempreature is too high" );
      }
      if (val>=100){
        
        Serial.println("Gass is high");
        delay(100);
        fona.sendSMS(callerIDbuffer,"Gas is high" );
      }

      /*if (smsString == "On")  //Change "On" to something secret like "On$@8765"
      {
        digitalWrite(RELAY_PIN, HIGH);
        Serial.println("LED is ON.");
        delay(100);
        fona.sendSMS(callerIDbuffer,"LED is ON." );
     
      }
      else if(smsString == "Off") //Change "On" to something secret like "Off&%4235"
      {
        digitalWrite(RELAY_PIN, LOW);
        Serial.println("LED is OFF.");
        delay(100);
        fona.sendSMS(callerIDbuffer, "LED is OFF.");
      }*/
       
      if (fona.deleteSMS(slot)) {
        Serial.println(F("OK!"));
      } else {
        Serial.print(F("Couldn't delete SMS in slot ")); Serial.println(slot);
        fona.print(F("AT+CMGD=?\r\n"));
      }
    }  
  }
}

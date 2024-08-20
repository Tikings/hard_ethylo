#include <Arduino.h>
#include <WiFi.h>
#include <tgmath.h>

// PINs
#define   ANALOG_MQ3      35
#define   PIN_LED_1       25
#define   PIN_LED_2       33
#define   PIN_LED_3_BUZZ  32
#define   PIN_BUTTON_1    34
#define   PIN_BUTTON_2    39

//PWM
#define   FREQ            1000
#define   LED_BUZZ_CHANNEL 0
#define   BIT_RESOLUTION  8

//Wifi
#define   WIFI_SSID       "Nik les stups"
#define   WIFI_PASSWORD   "fdpfdpfdp"

// BOOL States
bool WS_CONNECTED = 1;
bool LAST_SENT_DATA = 1;
bool NEW_BUTTON; 
bool OLD_BUTTON;
bool SENDING_DATA = 0;

// Working parameters of the breathalyzer  

static int TIME_BTW_READ=200; // Time before entering the "decision tree"
static long LAST_RECORD = 60000; // Initially setup to more than "HEATING TIME"s to allow the reading state at the beginning of the loop
static int TIME_READING = 7000; // Time to read (= blowing) -> Little bit long to check
static int HEATING_TIME = 10000; // Break before entering loop and after reading to let the sensor heat a bit (better reads)
static int TIME_READ_HOLD = 1000; // Time the button 1 need to be held to launch the read. 

// VARIABLES 
int COUNTER_ARR_VAL = 0; // To append values to the ARR_VAL array and to know hiw much value we stored
int TIME_HOLD = 0;  // Time the button have been held in ms
int VAL_MQ3;

// Create the array that will contain the recorded values 
int ARR_VAL[140]; // The maximum number of value that we will be able to store

// =============== FUNCTIONS ==========================================

void send_data() {
  Serial.println("======== Sending Data ========");
  Serial.println("======== Data sent =======");
  LAST_SENT_DATA = 1 ;
}



void connect_wifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("\n ====  Connecting WIFI ==== ");
  while(WiFi.status() != WL_CONNECTED){

      // Blink LED_1 while connecting
      digitalWrite(PIN_LED_1,HIGH);
      delay(50);
      digitalWrite(PIN_LED_1,LOW);

      Serial.print(".");
      delay(1000); 
  }
  Serial.print("Connected to : "); Serial.println(WIFI_SSID);
}




void connect_webSocket() {
  Serial.println("======= Connection WebSocket =======");
  Serial.println("======= WebSocket connected  =======");
}

// ===============================================================

void setup() {

  int nb_val = int(TIME_READING / TIME_BTW_READ);

  Serial.begin(115200);
  Serial.println("\n Starting Initialization");
  Serial.println("================");
  Serial.println("Setting Pin_mode");
  Serial.println("__________________");

  //MQ-3 Sensor
  pinMode(ANALOG_MQ3,INPUT);

  //LED Pins
  pinMode(PIN_LED_1,OUTPUT);
  pinMode(PIN_LED_2,OUTPUT);

  // LED 3 and BUZZER -> PWM modulation at the same time bc there is the buzzer on the pin (passive buzzer)
  ledcSetup(LED_BUZZ_CHANNEL,FREQ,BIT_RESOLUTION);
  ledcAttachPin(PIN_LED_3_BUZZ,LED_BUZZ_CHANNEL);

  //Pin Mode Button 
  pinMode(PIN_BUTTON_1,INPUT);
  // pinMode(PIN_BUTTON_2,INPUT); //That will be the EN if it is still there.

  Serial.println("Initialisation OK");
  Serial.println("===============");

  Serial.print("Testing Items");
  Serial.println("__________________");

  // LEDs 

  digitalWrite(PIN_LED_1,HIGH);
  digitalWrite(PIN_LED_2,HIGH);

  // ON : PWM 50% Duty
  ledcWrite(LED_BUZZ_CHANNEL,125); 

  delay(1000);

  digitalWrite(PIN_LED_1,LOW);
  delay(100);
  digitalWrite(PIN_LED_2,LOW);
  delay(100);

  // OFF : PWM 0% Duty
  ledcWrite(LED_BUZZ_CHANNEL,0);

  delay(100);

  Serial.println("Testing finished");
  Serial.println("===============");

  // Connect to the Wifi

  WiFi.mode(WIFI_STA);
  connect_wifi();

  digitalWrite(PIN_LED_1, HIGH); // To change when we add the WEB Socket connection 

  Serial.println("Connected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  connect_webSocket();

  delay(HEATING_TIME);
  Serial.println(" \n ============== SETUP FINISHED ============== \n");

}

void loop()
{
  // ===================================================
  // ==========Add LED_1 management (Wifi and WS)=======
  // ===================================================

  // Always happening
  VAL_MQ3 = analogRead(ANALOG_MQ3);
  delay(TIME_BTW_READ);
  LAST_RECORD += TIME_BTW_READ;

  // 1st branch
  if (WiFi.status() == WL_CONNECTED)
  {

    if (WS_CONNECTED){

      digitalWrite(PIN_LED_1, HIGH);

      if (LAST_RECORD < TIME_READING){

        // Recording the value
        ARR_VAL[COUNTER_ARR_VAL] = VAL_MQ3;
        COUNTER_ARR_VAL ++ ; 

        // LED Management
        ledcWrite(LED_BUZZ_CHANNEL,125);
        digitalWrite(PIN_LED_2,LOW);
      }

      else if((TIME_READING < LAST_RECORD) && (LAST_RECORD < (TIME_READING + HEATING_TIME))){

        // LED Management
        ledcWrite(LED_BUZZ_CHANNEL,0); // Read finished
        if(!SENDING_DATA){ // Filter to avoid sending data multiple times in the loop
          SENDING_DATA = 1 ; 
          send_data();
        }
      }

      else { // LAST_RECORD > TIME READING + HEATING TIME
        
        SENDING_DATA = 0;

        if(LAST_SENT_DATA){ // Recorded data have been sent to the server
        //Read Button state
        NEW_BUTTON = digitalRead(PIN_BUTTON_1);
        Serial.print("Button Value : "); Serial.println(NEW_BUTTON);
        Serial.print("Time Held : "); Serial.println(TIME_HOLD);
        
        digitalWrite(PIN_LED_2,HIGH);

        // This part : You need to old the button for $TIME_READ_HOLD ms to be able to record the values
        if((!NEW_BUTTON) && (!OLD_BUTTON)){
          TIME_HOLD += TIME_BTW_READ;
        }
        else {
          TIME_HOLD = 0;
        }

        if(TIME_HOLD > TIME_READ_HOLD){
          LAST_RECORD = 0;
        }

        }
        else {
          send_data(); 
        }
      }
    }
    else {connect_webSocket();}
  
  }
  else {connect_wifi();}
}

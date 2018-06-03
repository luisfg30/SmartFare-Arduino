// freeRTOS files
#include "Arduino_FreeRTOS.h"
#include "timers.h"
#include "event_groups.h"
// Standard C libraries
#include <stdio.h>
#include <time.h>
// Third part libraries
#include "Http.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "MFRC522.h"
#include "RTCtimeUtils.h"
#include "RtcDS1307.h"
#include "ArduinoJson.h"
// Arduino libraries
#include <Wire.h>
#include <SPI.h>
// Smartfare files
#include "SmartFareData.h"
#include "rfid_utils.h"
#include "SmartFareUtils.h"

/*******************************************************************************
 * Pin Configuration
 ******************************************************************************/
#define OLED_RESET          4
#define RFID_RST_PIN        48         
#define RFID_SS_PIN         53    //Slave Select or CS Chip Select
#define LED_RED             7        
#define LED_GREEN           6
#define LED_BLUE            5

#define GPS_BAUD_RATE       9600 
#define GSM_BAUD_RATE       9600
#define DEBUGSERIAL
#define DEBUGRFID
// #define DEBUGGPS

#define TIMER_PERIOD 5000 / portTICK_PERIOD_MS
#define SYNC_EVENT_BIT ( 1UL << 0UL )

/*******************************************************************************
 * Public types/enumerations/variables
 ******************************************************************************/

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  char jsonString[200];

TimerHandle_t syncTimer;
EventGroupHandle_t xEventGroup;

static UserData_t eventsBuffer[USER_BUFFER_SIZE];
static uint8_t eventsIndex = 0;

static uint32_t onBoardUsers[USER_BUFFER_SIZE];
static uint8_t onBoardIndex;

RtcDS1307<TwoWire> Rtc(Wire);
// Global flags
static bool RTC_started = 0;


// GPS variables
volatile uint8_t dollar_counter = 0;
volatile uint8_t NMEA_index = 0;
char parse_buffer[128];
bool GPRMC_received;
static bool NMEA_valid;
static GPS_data_t gps_data_parsed;

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);  // Create MFRC522 instance

Adafruit_SSD1306 display(OLED_RESET);


unsigned int SIM800_RX_PIN = 10; 
unsigned int SIM800_TX_PIN = 11;
unsigned int SIM800_RST_PIN = 12;
HTTP http(GSM_BAUD_RATE, SIM800_RX_PIN, SIM800_TX_PIN, SIM800_RST_PIN, TRUE);

// Tasks definition
void TaskGSM( void *pvParameters );
void TaskRFID( void *pvParameters );
void TaskGPS(void *pvParameters);

// aux functions declarations
void print(const __FlashStringHelper *message, int code = -1);

// the setup routine runs once when you press reset:
void setup() {

  Rtc.Begin();

  // Create ventGroup
  xEventGroup = xEventGroupCreate();

  // Create a oneshot timer
syncTimer = xTimerCreate("SyncTimer", TIMER_PERIOD , pdFALSE, 0, 
syncTimerCallback );

  // Init LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

// Debug Serial port
  #ifdef DEBUGSERIAL
    Serial.begin(115200);
    while(!Serial);
    Serial.println(F("Started debug serial!"));
  #endif
// GPS serial port
  Serial1.begin(GPS_BAUD_RATE); 
  while(!Serial1);
  Serial.println(F("Started serial 1 (GPS)"));


  // initialize with the I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  

  SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
  #ifdef DEBUGRFID
    mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  #endif
  // First message
  setLedRGB(1,1,0);
  displayText("Iniciando RTC", 2); 

  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest
  xTaskCreate(TaskGSM , (const portCHAR *)"GSM", 1024, NULL, 3, NULL);
  xTaskCreate(TaskRFID, (const portCHAR *) "RFID", 512, NULL, 2,NULL);
  xTaskCreate(TaskGPS, (const portCHAR *) "GPS", 512 , NULL, 1, NULL);
  vTaskStartScheduler();
}

void loop() {
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


void TaskGPS(void *pvParameters) {
(void) pvParameters;

char incomingByte = 0;
#ifdef DEBUGSERIAL   
  Serial.println(F("task GPS created"));
#endif

  for(;;) {

    if (Serial1.available() > 0) {
      incomingByte = Serial1.read();
      #ifdef DEBUGGPS 
        Serial.print(incomingByte);
      #endif
      if (incomingByte == '$') {
        if(dollar_counter > 0) {
          	parse_string();
						memset(parse_buffer, 0 , sizeof(parse_buffer));
						NMEA_index = 0;
            if(GPRMC_received == 1) {
              if (NMEA_valid == 1) {
                if (RTC_started == 0) {
                  time_t startTime = setRTCTime();
                  Rtc.SetTime(&startTime);
                  RTC_started = 1;
                }
                #ifdef DEBUGGPS 
                  setLedRGB(0,0,1);
                  vTaskDelay( 100 / portTICK_PERIOD_MS );
                  setLedRGB(1,1,0);
                  displayGPSData();
                #endif
              } else {
                  if (RTC_started == 0) {
                    setLedRGB(1,0,0);
                    vTaskDelay( 100 / portTICK_PERIOD_MS );
                    setLedRGB(1,1,0); 
                  }
              }
            } 
        }
        dollar_counter++;
      }
      parse_buffer[NMEA_index] = incomingByte;
      NMEA_index++;
    }
  }
}

void TaskGSM(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
   Serial.println(F("task GSM created"));
  
  char response[32];
  char body[90];
  Result result;
   

  EventBits_t xEventGroupValue;
  const EventBits_t xBitsToWaitFor = SYNC_EVENT_BIT;

  for (;;) 
  {
    // Block to wait for event bits to become set within the event group
    xEventGroupValue = xEventGroupWaitBits(
    xEventGroup,
    xBitsToWaitFor,
    // Clear bits on exit if the unblock condition is met.
    pdTRUE,
    // Don't wait for all bits
    pdFALSE,
    // Don't time out. Wait forever
    portMAX_DELAY );

    if( ( xEventGroupValue & SYNC_EVENT_BIT ) != 0 ) {
      setLedRGB(1,0,1);
      displayText("Sincronizando", 2);

      // Configure connection
      print(F("Cofigure bearer: "), http.configureBearer("claro.com.br"));
      result = http.connect();
      print(F("HTTP connect: "), result);

      // Add events to JSON object
      int i;
      char s_userId[12];
      char s_balance[12];
      char s_eventType[2];

      // eventsBuffer
      for(i = 0; i < USER_BUFFER_SIZE ; i++ ){
        if (eventsBuffer[i].userId != 0) {
          sprintf(s_userId, "%lu", eventsBuffer[i].userId);
          sprintf(s_balance, "%d", eventsBuffer[i].balance);
          sprintf(s_eventType, "%d",eventsBuffer[i].eventType);
          root["eventType"] = s_eventType;
          root["userID"] = s_userId;
          root["balance"] = s_balance;
          root["timestamp"] = eventsBuffer[i].timestamp;
          root["latitude"] = eventsBuffer[i].latitude;
          root["longitude"] = eventsBuffer[i].longitude;

          root.printTo(jsonString);
          result = http.post("ptsv2.com/t/1etbw-1520389850/post", jsonString, response);
          print(F("HTTP POST: "), result);
          if (result == SUCCESS) {
            #ifdef DEBUGSERIAL
              Serial.println(F("Server Response:"));
              Serial.println(response);
            #endif  
          }
        }
      }
  print(F("HTTP disconnect: "), http.disconnect());
    }
  }
}

void TaskRFID(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  UserData_t lastUserData;
  char balanceString[6];

  Serial.println(F("task RFID created"));
  for (;;)
  {
      setLedRGB(0,0,0);
      displayText("Aproxime o cartao", 2);
      // Look for new cards in RFID2
      if (mfrc522.PICC_IsNewCardPresent()) {
        // Select one of the cards
        if (mfrc522.PICC_ReadCardSerial()) {
          lastUserData.balance = readCardBalance(mfrc522);
          if (lastUserData.balance != (-999)) {
            displayBalance(lastUserData.balance);
            setLedRGB(0, 1, 0);
          } else {
            displayText("Cartao naocadastrado", 2);
            setLedRGB(0, 0, 1);
          }
          //int status = writeCardBalance(mfrc2, 9999); // used to recharge the card

          // Convert the uid bytes to an integer, byte[0] is the MSB
          lastUserData.userId =
            (uint32_t)mfrc522.uid.uidByte[3] |
            (uint32_t)mfrc522.uid.uidByte[2] << 8 |
            (uint32_t)mfrc522.uid.uidByte[1] << 16 |
            (uint32_t)mfrc522.uid.uidByte[0] << 24;
           #ifdef DEBUGRFID
            Serial.print(F("\n\nCard uid bytes: "));
            dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.print(F(" UID: "));  
            Serial.println(lastUserData.userId);
           #endif
            
            // Record timestamp
            if (Rtc.IsDateTimeValid())      
            {
              time_t now = Rtc.GetTime();
              struct tm utc_tm;
              // Standard ISO timestamp yyy-mm-dd hh:mm:ss
              gmtime_r(&now, &utc_tm);      
              char utc_timestamp[20];
              strcpy(utc_timestamp, isotime(&utc_tm));
              strcpy(lastUserData.timestamp,utc_timestamp); 
              #ifdef DEBUGSERIAL
                Serial.print(F("UTC timestamp: "));
                Serial.println(lastUserData.timestamp);
              #endif
            }

           // Record coordinates
           strcpy(lastUserData.latitude, gps_data_parsed.latitude); 
           strcat(lastUserData.latitude, gps_data_parsed.latituteSign);
           strcpy(lastUserData.longitude, gps_data_parsed.longitude);
           strcat(lastUserData.longitude, gps_data_parsed.longitudeSign);

           #ifdef DEBUGRFID
            Serial.print(F("balance: ")); Serial.println(lastUserData.balance);
            Serial.print(F("lat: ")); Serial.println(lastUserData.latitude);
            Serial.print(F("long: ")); Serial.println(lastUserData.longitude);
           #endif

          // Check if user is on vehicle
          int userIndex = getUserByID(lastUserData.userId, onBoardUsers);
          Serial.print("userIndex: "); Serial.println(userIndex);
          if( userIndex == -1) {
            // Add user to onBoardUsers buffer;
            onBoardUsers[onBoardIndex] = lastUserData.userId;
            onBoardIndex++;
            onBoardIndex %= USER_BUFFER_SIZE;

            // tapIn event
            lastUserData.eventType = 0;
          }
          else {
            // tapOut event
            lastUserData.eventType = 1;

            // Remove user from onBoardUsers buffer;
            if (userIndex != onBoardIndex - 1){// not in the last position
              // shift all other elements left
              uint8_t i; 
              for (i = userIndex ; i < onBoardIndex - 1; i ++){
                onBoardUsers[i] = onBoardUsers[i + 1];
              }
            }
            onBoardIndex --;
            onBoardUsers[onBoardIndex] = 0;

            if(lastUserData.balance != (-999) ){
              // Calculate fare
              // Write new balance to card;
              displayText("tarifa:",2);
            }
          }

          // Add user to event buffer
          eventsBuffer[eventsIndex] = lastUserData;
          eventsIndex++;
          eventsIndex %= USER_BUFFER_SIZE;

          #ifdef DEBUGRFID
            Serial.print("onBoardIndex: ");Serial.print(onBoardIndex); 
            Serial.print(" eventsIndex: "); Serial.print(eventsIndex);

            uint8_t j;
            for (j = 0; j < USER_BUFFER_SIZE; j++) {
              Serial.print(" ");Serial.print(onBoardUsers[j]);
            }
          #endif
        }
      // Reset SyncTimer
      xTimerStart(syncTimer, 0);
      }
      // Wait for another user card
      vTaskDelay( 1500 / portTICK_PERIOD_MS ); 
  }
}

/**
 * @brief Set an eventGRoup bit to wake up the GSM task
 * 
 * @param xTimer 
 */
static void syncTimerCallback( TimerHandle_t xTimer )
{
  xEventGroupSetBits( xEventGroup, SYNC_EVENT_BIT );
}

/*--------------------------------------------------*/
/*---------------------- Extra functions ---------------------*/
/*--------------------------------------------------*/



void print(const __FlashStringHelper *message, int code = -1){
  if (code != -1){
    Serial.print(message);
    Serial.println(code);
  }
  else {
    Serial.println(message);
  }
}

void displayText(char* text, uint8_t textSize) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(textSize);
  display.setTextColor(WHITE);
  display.print(text);
  display.display();
}

void displayBalance(int balance) {
  char balanceString[16];
  if(balance < 0) {
    sprintf(balanceString , "R$ -%d,%.2d", 0-balance/100, 0-balance%100);
  } else {
      sprintf(balanceString, "R$ %d,%.2d", balance/100, balance%100);
  }
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.println("Saldo: ");
  display.print(balanceString);
  display.display();
}

void setLedRGB(uint8_t r, uint8_t g, uint8_t b) {
  if (r == 1) {
    digitalWrite(LED_RED, HIGH); 
  } else if(r == 0) {
    digitalWrite(LED_RED, LOW);
  }
  if (g == 1) {
    digitalWrite(LED_GREEN, HIGH);
  } else if(g == 0) {
    digitalWrite(LED_GREEN, LOW);
  }
  if (b == 1) {
    digitalWrite(LED_BLUE, HIGH);
  } else if(b == 0) {
    digitalWrite(LED_BLUE, LOW);
  }
}


void parse_string() {

char *pToken;
char *pToken1;
char timeStamp[10];
uint8_t field_counter = 0;
bool breakLoop = 0;

    if(strstr(parse_buffer,"GPRMC")) {
        GPRMC_received = 1;
        pToken = strtok (parse_buffer, ",");
        while (pToken != NULL)
          {
            if(breakLoop == 1) {
              break;
            }
            // Serial.println(pToken);
            switch(field_counter) {

              case 1: // Timestamp
                // memcpy(&timeStamp,&pToken,strlen(pToken+1));
                strncpy(timeStamp, pToken, strlen(pToken));
              break;

              case 2: // Data valid?
                if(strcmp(pToken,"A") == 0) {
                  NMEA_valid = 1;
                } else {
                  NMEA_valid = 0;
                  breakLoop = 1;
                }
              break;

              case 3: // Latitude
                if(NMEA_valid == 1) {
                  strncpy(gps_data_parsed.latitude,pToken, strlen(pToken));
                }
              break;
              case 4: // Latitude sign
                if(NMEA_valid == 1) {
                  strncpy(gps_data_parsed.latituteSign,pToken, strlen(pToken));
                }
              break;
              case 5: // Longitude 
                if(NMEA_valid == 1) {
                  strncpy(gps_data_parsed.longitude,pToken, strlen(pToken));
                }
              break;
              case 6: // Longitude sign
                if(NMEA_valid == 1) {
                  strncpy(gps_data_parsed.longitudeSign,pToken, strlen(pToken));
                }
              break;
              case 8: // Date
                if(NMEA_valid == 1) {
                  strncpy(gps_data_parsed.date,pToken, strlen(pToken));
                }
              break;
            }
            pToken = strtok (NULL, ",");
            field_counter ++;
          }
          if(NMEA_valid == 1) {
            // Remove the .ss from timeStamp
            pToken1 = strtok(timeStamp,".");
            if (pToken1 != NULL) {
              strncpy(gps_data_parsed.timestamp, pToken1, strlen(pToken1));
            }
          }
    } else {
      GPRMC_received = 0;
    }
}

/**
 * @brief Show parsed data at the OLED display
 * 
 */
void displayGPSData() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.print(gps_data_parsed.timestamp);
  display.setCursor(0,15);
  display.print(gps_data_parsed.latitude);
  display.print(gps_data_parsed.latituteSign);
  display.setCursor(0,30);
  display.print(gps_data_parsed.longitude);
  display.print(gps_data_parsed.longitudeSign);
  display.setCursor(0,45);
  display.print(gps_data_parsed.date);

  display.display();
}

time_t setRTCTime() {

  struct tm tm_time;
	int year, month, day, hours, minutes, seconds;
  char* pDateString = gps_data_parsed.date;
  char* pTimeString = gps_data_parsed.timestamp;
  char s_day[3];
  char s_month[3];
  char s_year[3];
  char s_hour[3];
  char s_min[3];
  char s_sec[3];

  strncpy(s_day, pDateString, 2);
  pDateString += 2;
  strncpy(s_month, pDateString, 2);
  pDateString += 2;
  strncpy(s_year, pDateString, 2);

  strncpy(s_hour, pTimeString, 2);
  s_hour[2] = '\0';
  pTimeString += 2;
  strncpy(s_min, pTimeString, 2);
  s_min[2] = '\0';
  pTimeString += 2;
  strncpy(s_sec, pTimeString, 2);
  s_sec[2] = '\0';

	year = (int) strtol(s_year, NULL, 10);	
	month = (int) strtol(s_month, NULL, 10);	
	day = (int) strtol(s_day, NULL, 10);	
	hours = (int) strtol(s_hour, NULL, 10);	
	minutes = (int) strtol(s_min, NULL, 10);	
	seconds = (int) strtol(s_sec, NULL, 10);

  tm_time.tm_year = year + 2000 - 1900;
  tm_time.tm_mon = month - 1;
  tm_time.tm_mday = day;
  tm_time.tm_hour = hours;
  tm_time.tm_min  = minutes;
  tm_time.tm_sec  = seconds;
  tm_time.tm_isdst = 0;
  return mktime(&tm_time);
}


/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}
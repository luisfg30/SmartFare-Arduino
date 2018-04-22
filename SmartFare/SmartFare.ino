
#include "Http.h"
#include "Arduino_FreeRTOS.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include <SPI.h>
#include <MFRC522.h>
#include <stdio.h>
#include "SmartFareData.h"


/*******************************************************************************
 * Pin Configuration
 ******************************************************************************/
#define OLED_RESET          4
#define RFID_RST_PIN        48         
#define RFID_SS_PIN         53    //Slave Select or CS Chip Select
#define LED_RED             22        
#define LED_GREEN           24
#define LED_BLUE            26

#define GPS_BAUD_RATE       9600 

/*******************************************************************************
 * Public types/enumerations/variables
 ******************************************************************************/

// Global flags

bool RTC_started = 0;

// GPS variables
volatile uint8_t dollar_counter = 0;
volatile uint8_t NMEA_index = 0;
char parse_buffer[128];
bool GPRMC_received;
bool NMEA_valid;
GPS_data_t gps_data_valid;
GPS_data_t gps_data_parsed;
static char testString[20];

char printString [120];

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);  // Create MFRC522 instance

Adafruit_SSD1306 display(OLED_RESET);


unsigned int SIM800_RX_PIN = 10; 
unsigned int SIM800_TX_PIN = 11;
unsigned int SIM800_RST_PIN = 12;
HTTP http(9600, SIM800_RX_PIN, SIM800_TX_PIN, SIM800_RST_PIN, TRUE);

// Tasks definition
void TaskGSM( void *pvParameters );
void TaskRFID( void *pvParameters );
void TaskGPS(void *pvParameters);

// aux functions declarations
void print(const __FlashStringHelper *message, int code = -1);

// the setup routine runs once when you press reset:
void setup() {

  // Init LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

// Debug Serial port
  Serial.begin(9600);
  while(!Serial);
  Serial.println("Starting debug serial!");

// GPS serial port
  Serial1.begin(GPS_BAUD_RATE); 
  while(!Serial1);
  Serial.println("Starting serial 1!");

  // initialize with the I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  displayTest();  

  SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  // First message
    setLedRGB(1,1,0);
    displayText("Iniciando RTC"); 

   // Now set up two tasks to run independently.
  xTaskCreate(
    TaskGSM
    ,  (const portCHAR *)"GSM"   // A name just for humans
    ,  512  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskRFID
    ,  (const portCHAR *) "RFID"
    ,  128  // Stack size
    ,  NULL
    ,  2  // Priority
    ,  NULL );

      xTaskCreate(
    TaskGPS
    ,  (const portCHAR *) "GPS"
    ,  256  // Stack size
    ,  NULL
    ,  3  // Priority
    ,  NULL );

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

// the loop routine runs over and over again forever:
void loop() {
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


void TaskGPS(void *pvParameters) {
(void) pvParameters;

char incomingByte = 0;   
Serial.println("task GPS created");

  for(;;) {

    if (Serial1.available() > 0) {
      incomingByte = Serial1.read();
      Serial.print(incomingByte);
      if (incomingByte == '$') {
        if(dollar_counter > 0) {
          	parse_string();
						memset(parse_buffer, 0 , sizeof(parse_buffer));
						NMEA_index = 0;
            if(GPRMC_received == 1) {
              if (NMEA_valid == 1) {
                Serial.println("NMEA OK");
                setLedRGB(0,0,1);
                vTaskDelay( 100 / portTICK_PERIOD_MS );
                setLedRGB(1,1,0);
                printGPSData();            
              } else {
                Serial.println("NMEA INVALIDO");
                setLedRGB(1,0,0);
                vTaskDelay( 100 / portTICK_PERIOD_MS );
                setLedRGB(1,1,0);      
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
  Serial.println("task 1 created");
  // testPOST();


  for (;;) // A Task shall never return or exit.
  {
            
  }
}

void TaskRFID(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  
  Serial.println("task 2 created");
  for (;;)
  {
      // Look for new cards in RFID2
      if (mfrc522.PICC_IsNewCardPresent()) {
        // Select one of the cards
        if (mfrc522.PICC_ReadCardSerial()) {
          //int status = writeCardBalance(mfrc2, 9999); // used to recharge the card
           Serial.println("\n\nCARD FOUND");
           digitalWrite(LED_BLUE, HIGH);  
           vTaskDelay( 100 / portTICK_PERIOD_MS );                
           digitalWrite(LED_BLUE, LOW);   
        }
      }
      vTaskDelay( 1000 / portTICK_PERIOD_MS ); // wait for one second
  }
}

/*--------------------------------------------------*/
/*---------------------- Extra functions ---------------------*/
/*--------------------------------------------------*/

void testPOST(){
  
  char response[32];
  char body[90];
  Result result;

//  print(F("Cofigure bearer: "), http.configureBearer("zap.vivo.com.br"));
  print(F("Cofigure bearer: "), http.configureBearer("claro.com.br"));
  result = http.connect();
  print(F("HTTP connect: "), result);

  
  result = http.post("ptsv2.com/t/1etbw-1520389850/post", "{\"date\":\"12345678\"}", response);
  print(F("HTTP POST: "), result);
  if (result == SUCCESS) {
    Serial.println("Server Response:");
    Serial.println(response);
  }

  print(F("HTTP disconnect: "), http.disconnect());
}


void print(const __FlashStringHelper *message, int code = -1){
  if (code != -1){
    Serial.print(message);
    Serial.println(code);
  }
  else {
    Serial.println(message);
  }
}


void displayTest(){

  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();
    // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello, world!");
  display.setTextColor(BLACK, WHITE); // 'inverted' text
  display.println(3.141592);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.print("0x"); display.println(0xDEADBEEF, HEX);
  display.println("R$ : 0123456789");
  display.display();
  delay(2000);
  display.clearDisplay();
}


void displayText(char* text) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.print(text);
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


void parse_string(){

char *pToken;
char *pToken1;
char *timeStamp;
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
            Serial.println(pToken);
            switch(field_counter) {

              case 1:
                memcpy(&timeStamp,&pToken,strlen(pToken+1));
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
                  memcpy(gps_data_parsed.latitude,&pToken, strlen(pToken)+1);
                }
              break;
              case 4: // Latitude sign
                if(NMEA_valid == 1) {
                  memcpy(gps_data_parsed.latituteSign,&pToken, strlen(pToken)+1);
                }
              break;
              case 5: // Longitude 
                if(NMEA_valid == 1) {
                  memcpy(gps_data_parsed.longitude,&pToken, strlen(pToken)+1);
                }
              break;
              case 6: // Longitude sign
                if(NMEA_valid == 1) {
                  memcpy(gps_data_parsed.longitudeSign,&pToken, strlen(pToken)+1);
                }
              break;
              case 8: // Date
                if(NMEA_valid == 1) {
                  memcpy(gps_data_parsed.date,&pToken, strlen(pToken)+1);
                }
              break;
            }
            pToken = strtok (NULL, ",");
            field_counter ++;
          }
          if(NMEA_valid == 1) {
            // Remove the .ss from timeStamp
            Serial.println(timeStamp);
            pToken1 = strtok(timeStamp,".");
            if (pToken1 != NULL) {
              Serial.println(timeStamp);
              Serial.println(pToken1);
              strcpy(testString, pToken1);
            }
          }
    } else {
      GPRMC_received = 0;
    }
}

void printGPSData() {
  Serial.println("GPS_DATA_PARSED:\n");
  Serial.write(testString);
  displayText(testString);
  Serial.println("");
  Serial.write(gps_data_parsed.latitude);
  Serial.write(gps_data_parsed.latituteSign);
  Serial.println("");
  Serial.write(gps_data_parsed.longitude);
  Serial.write(gps_data_parsed.longitudeSign);
  Serial.println("");
  Serial.write(gps_data_parsed.date);
}
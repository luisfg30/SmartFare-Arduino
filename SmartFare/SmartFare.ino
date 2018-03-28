
#include "Http.h"
#include "Arduino_FreeRTOS.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include <SPI.h>
#include <MFRC522.h>



/*--------------------------------------------------*/
/*----------- Global variables ------------------t---*/
/*--------------------------------------------------*/
#define OLED_RESET          4
#define RFID_RST_PIN        48         
#define RFID_SS_PIN         53    //Slave Select or CS Chip Select
#define LED_RED             22        
#define LED_GREEN           24
#define LED_BLUE            26           

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);  // Create MFRC522 instance

Adafruit_SSD1306 display(OLED_RESET);


unsigned int SI800_RX_PIN = 10; // Arduino Mega uses pin 10 por RX
unsigned int SI800_TX_PIN = 11;
unsigned int SI800_RST_PIN = 12;
HTTP http(9600, SI800_RX_PIN, SI800_TX_PIN, SI800_RST_PIN, TRUE);

// define two tasks for Blink & AnalogRead
void TaskBlink( void *pvParameters );
void TaskAnalogRead( void *pvParameters );

// aux functions declarations
void print(const __FlashStringHelper *message, int code = -1);

// the setup routine runs once when you press reset:
void setup() {

  // Init LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  Serial.begin(9600);
  while(!Serial);
  Serial.println("Starting!");

  // initialize with the I2C addr 0x3D (for the 128x64)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  displayTest();  

  SPI.begin();			// Init SPI bus
	mfrc522.PCD_Init();		// Init MFRC522
	mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
	Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

   // Now set up two tasks to run independently.
  xTaskCreate(
    TaskBlink
    ,  (const portCHAR *)"Blink"   // A name just for humans
    ,  512  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskAnalogRead
    ,  (const portCHAR *) "AnalogRead"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
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

void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  Serial.println("task 1 created");
  // testPOST();


  for (;;) // A Task shall never return or exit.
  {
    digitalWrite(LED_RED, HIGH);  
    vTaskDelay( 1000 / portTICK_PERIOD_MS );                
    digitalWrite(LED_RED, LOW);   
    vTaskDelay( 1000 / portTICK_PERIOD_MS );   
    digitalWrite(LED_GREEN, HIGH);   
    vTaskDelay( 1000 / portTICK_PERIOD_MS );                      
    digitalWrite(LED_GREEN, LOW);
  }
}

void TaskAnalogRead(void *pvParameters)  // This is a task.
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

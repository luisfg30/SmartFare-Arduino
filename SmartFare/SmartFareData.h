#include <stdio.h>
#ifndef INC_SMARTFAREDATA_H_
#define INC_SMARTFAREDATA_H_

#define USER_BUFFER_SIZE 10
#define MIN_BALANCE 300
#define VEHICLE_ID 011543 	// last 6 numbers of VIN

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

// Unformated GPS NMEA data
typedef struct {
	char timestamp[12];
	char latitude[12];
	char latituteSign[2];
	char longitude[12];
	char longitudeSign[2];
	char date[6];
} GPS_data_t;

// Structure used to store user Data
typedef struct {
	uint32_t userId; 	// The PICC used stores a 4 byte user ID
 	int balance;  		// The user balance in cents, may be negative
	char timestamp[20]; //UTC timestamp
	char latitude[15];
	char longitude[15];	
	uint8_t eventType; 	// 0 = tapIn, 1 = tapOut	
	uint8_t synchronized; // 0 = not synch, 1 = synched 		 
} UserData_t;

// Used to show proper massages in the LCD displays
typedef enum __UserStatus {

	START_MESSAGE,
	USTATUS_UNAUTHORIZED,  // user is already on board
	USTATUS_INSUF_BALANCE, // user have insufficient balance
	USTATUS_AUTHORIZED,	// user have enough balance
	USTATUS_TAP_OUT,	   // user get out of the vehicle, new_balance >
						   // minimum_balance
	USTATUS_TAP_OUT_LOW_BALANCE
} UserStatus_t;

#endif /* INC_SMARTFAREDATA_H_ */

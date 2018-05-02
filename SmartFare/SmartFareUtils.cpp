#include "SmartFareUtils.h"

int getUserByID(uint32_t userId, UserInfo_T *usersBuffer) {

	int i;

	for (i = 0; i < USER_BUFFER_SIZE; i++) {
		if (userId == usersBuffer[i].userId) {
			return i;
		}
	}

	return -1;
}

void addNewUser(uint32_t userId, UserInfo_T userData) {
	// //create new user struct
	// UserInfo_T new_user;

	// //assign initial values
	// new_user.userId = userId;

	// // add user to userInfoArray
	// usersBuffer[usersBufferHead] = new_user;
	// usersBufferHead ++;

	// if (usersBufferHead > USER_BUFFER_SIZE - 1) {
	// 	// save buffer in other safe place
	// 	// overwrite buffer values
	// 	usersBufferHead = 0;
	// }
}

void removeUser (uint32_t userId) {

// 		int userIndex = getUserByID(userId);
// 		usersBuffer[userIndex].outTimestamp = RTC_getFullTime();

// 		if(simulated_data == false){
// 			//save data in user structure
// 			usersBuffer[userIndex].distance = odometer_Value -
// 					usersBuffer[userIndex].inOdometerMeasure;
// 			usersBuffer[userIndex].outOdometerMeasure = odometer_Value;
// 			usersBuffer[userIndex].outLatitude = latitude;
// 			usersBuffer[userIndex].outLongitude = longitude;
// 		}

// 		// Calculate fare based on vehicle movement and update user data
// 		int fare = calculateFare(userId);

// 		// Update user balance 
// 		int new_balance = usersBuffer[userIndex].balance - fare;
// 		usersBuffer[userIndex].balance = new_balance;

// 		//update balance in the user card
// 		writeCardBalance(mfrc2, new_balance);

// 		//show message in LCD
		
// 		set_lcd_balance(new_balance);
// 		set_lcd_travel_fare(fare);
// 		change_lcd_message(USTATUS_TAP_OUT);

// //		send user data in JSON format
// 		userInfoCopy = usersBuffer[userIndex];


// 		// Remove user from buffer. Must improve this Data structure and 
// 		//algorithm latter.

// 		userIndex = getUserByID(userId);
// 		usersBuffer[userIndex].userId = 0;

// 		if (userIndex != usersBufferHead - 1){//not in the last position

// 			//shift all other elements left
// 			int i; 
// 			for (i = userIndex ; i < usersBufferHead - 1; i ++){
// 				usersBuffer[i] = usersBuffer[i + 1];
// 			}
// 		}
}

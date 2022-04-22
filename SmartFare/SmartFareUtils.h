

#ifndef INC_SMARTFAREUTILS_H_
#define INC_SMARTFAREUTILS_H_

#include "SmartFareData.h"
#include "ArduinoJson.h"

#define MAX_EVENTS 10
#define JSON_SIZE 2000 
/**
 * Search for a given userID, returns an index to it if found, -1 otherwise
 * @param  userID the iD to search for
 * @return        the index of the user in the usersBuffer
 */
int getUserByID(uint32_t userId, uint32_t *usersBuffer);

/**
 * Create an UserData_t instance with boarding data and stores it int the
 * usersBuffer
 * @param userID user unique identification number in the system
 */
void addNewUser(uint32_t userId);

/**
 * Remove a user from the usersBuffer after updating the vehicle data and 
 * calculated fare
 * @param userId userID user unique identification number in the system
 */
void removeUser (uint32_t userId);

#endif /* INC_SMARTFAREUTILS_H_ */
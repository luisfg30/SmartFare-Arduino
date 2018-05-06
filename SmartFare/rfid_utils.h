
#ifndef INC_RFID_UTILS_H
#define INC_RFID_UTILS_H

#include "MFRC522.h"


/**
 * Function to read the balance, the balance is stored in the block 4 (sector
 * 1), the first 4 bytes
 * @param  mfrc    MFRC522 object
 * @return         the balance is stored in the PICC, -999 if reading errors
 */
int readCardBalance(MFRC522 mfrc);

/**
 * Function to write the balance, the balance is stored in the block 4 (sector
 * 1), the first 4 bytes
 * @param  mfrc    MFRC522 object
 * @param  newBalance Desired balance to write to card.
 * @return            0 if no errors
 */
int writeCardBalance(MFRC522 mfrc, int newBalance);

#endif

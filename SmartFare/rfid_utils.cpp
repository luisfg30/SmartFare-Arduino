/*
* rfid_utils.c - Auxiliary library to use time delays 
* NOTE: Please also check the comments in rfid_utils.h - they provide useful hints
* and background information.
*/
#include "rfid_utils.h"

/****************************************
 * Private variables
 ****************************************/

// auxBuffer used to store the read end write data, each block have 16 bytes,
// auxBuffer must have 18 slots, see MIFARE_Read()
static uint8_t auxBuffer[18];
static uint8_t size = sizeof(auxBuffer);

// return status from MFRC522 functions
static MFRC522::StatusCode status;


/****************************************
 * Private Functions
 ****************************************/

/**
 * Read 16 bytes from a block inside a sector
 * @param  mfrc    MFRC522 object
 * @param  sector    card sector, 0 to 15
 * @param  blockAddr card block address, 0 to 63
 * @return           0 is no error, -1 is authentication error -2 is read error
 */
static int readCardBlock(MFRC522 mfrc, uint8_t sector,
						 uint8_t blockAddr) {

	MFRC522::MIFARE_Key key;
	int i;
	// using FFFFFFFFFFFFh which is the default at chip delivery from the
	// factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

	// Authenticate using key A
	status = (MFRC522::StatusCode) mfrc.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
										  blockAddr, &key, &(mfrc.uid));
	if (status != MFRC522::STATUS_OK) {
		return -1;
	}

	// Read data from the block
	status = (MFRC522::StatusCode) mfrc.MIFARE_Read( blockAddr, auxBuffer, &size);
	if (status != MFRC522::STATUS_OK) {
		return -2;
	}

	// Halt PICC
    mfrc.PICC_HaltA();
    // Stop encryption on PCD
    mfrc.PCD_StopCrypto1();

	return 0;
}

/**
 * Write 16 bytes to a block inside a sector
 * @param  mfrc    MFRC522 object
 * @param  sector    card sector, 0 to 15
 * @param  blockAddr card block address, 0 to 63
 * @return           0 is no error, -1 is authentication error -2 is write error
 */
static int writeCardBlock(MFRC522 mfrc, uint8_t sector,
						  uint8_t blockAddr) {

	MFRC522::MIFARE_Key key;
	int i;
	// using FFFFFFFFFFFF which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

	// Authenticate using key A
	status = (MFRC522::StatusCode) mfrc.PCD_Authenticate( MFRC522::PICC_CMD_MF_AUTH_KEY_A,
										  blockAddr, &key, &(mfrc.uid));
	if (status != MFRC522::STATUS_OK) {
		return -1;
	}

	// Write data from the block, always write 16 bytes
	status = (MFRC522::StatusCode) mfrc.MIFARE_Write( blockAddr, auxBuffer, 16);
	if (status != MFRC522::STATUS_OK) {
		return -2;
	}

	// Halt PICC
    mfrc.PICC_HaltA();
    // Stop encryption on PCD
    mfrc.PCD_StopCrypto1();

	return 0;
}

/****************************************
 * Public Functions
 ****************************************/

/**
 * Function to read the balance, the balance is stored in the block 4 (sector
 * 1), the first 4 bytes
 * @param  mfrc MFRC522 object
 * @return         the balance is stored in the PICC, -999 for reading errors
 */
int readCardBalance(MFRC522 mfrc) {

	int balance;

	int readStatus = readCardBlock(mfrc, 1, 4);

	if (readStatus == 0) {
		// convert the balance bytes to an integer, byte[0] is the MSB
		balance = (int)auxBuffer[3] | (int)(auxBuffer[2] << 8) |
				  (int)(auxBuffer[1] << 16) | (int)(auxBuffer[0] << 24);
	} else {
		balance = -999;
	}

	return balance;
}

/**
 * Function to rwrite the balance, the balance is stored in the block 4 (sector
 * 1), the first 4 bytes
 * @param  mfrc    MFRC522 object
 * @param  newBalance the balance to be write  i  the card
 * @return            0 is no errors
 */
int writeCardBalance(MFRC522 mfrc, int newBalance) {

	// set the 16 bytes auxBuffer, auxBuffer[0] is the MSB
	auxBuffer[0] = newBalance >> 24;
	auxBuffer[1] = newBalance >> 16;
	auxBuffer[2] = newBalance >> 8;
	auxBuffer[3] = newBalance & 0x000000FF;
	int i;
	for (i = 4; i < size; i++) {
		auxBuffer[i] = 0xBB;
	}

	int writeStatus = writeCardBlock(mfrc, 1, 4);

	return writeStatus;
}

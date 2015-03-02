#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

#define	MODEL_QUERY			0x80
#define	SERIAL_ERROR		0x55
#define	NFC_TAGQUERY		0x82
#define	NFC_TAGQUERY_UID	0x84
#define	NFC_ARDUINO			0x02
#define	NFC_NOTAG			0x81

#define	NFC_TYPE_PROFESSOR	0x00
#define	NFC_TYPE_STUDENT	0x01

boolean	tagDetected;
boolean	tagType;

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);

void setup(void)
{
	int ser;

	Serial.begin(115200);

	nfc.begin();

	uint32_t versiondata = nfc.getFirmwareVersion();
	if(!versiondata)
	{
		Serial.print(SERIAL_ERROR);
		while (1);
	}  

	nfc.SAMConfig();
  
	tagDetected = false;

	do
	{
		while(!(Serial.available() > 0));
		ser = Serial.read();
		if(ser == MODEL_QUERY)
			Serial.print(NFC_ARDUINO);
		else
			Serial.print(SERIAL_ERROR);
	}while(ser != MODEL_QUERY);
}


void loop(void)
{
	uint8_t success;
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
	uint8_t uidLength;
	uint8_t data1[16];
	uint8_t data2[16];
	uint8_t dataf[8];
	int ser;
    
	success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

	if(success && !tagDetected)
	{
		if(uidLength == 4)
		{
			uint8_t keya[6] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 };
			success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 44, 0, keya);  
			if(success)
			{
				success = nfc.mifareclassic_ReadDataBlock(44, data1);
		
				if(success)
				{
					//nfc.PrintHexChar(data1, 16);
					success = nfc.mifareclassic_ReadDataBlock(45, data2);

					if(success)
					{
						tagDetected = true;
						if(data1[4] == data1[15] &&
						data1[5] == data2[0] &&
						data1[6] == data2[1] &&
						data1[7] == data2[2] &&
						data1[8] == data2[3] &&
						data1[9] == data2[4])
						{
							tagType = NFC_TYPE_PROFESSOR;
							dataf[0] = data1[4];
							dataf[1] = data1[5];
							dataf[2] = data1[6];
							dataf[3] = data1[7];
							dataf[4] = data1[8];
							dataf[5] = data1[9];
						}
						else
						{
							tagType = NFC_TYPE_STUDENT;
							dataf[0] = data1[15];
							dataf[1] = data2[0];
							dataf[2] = data2[1];
							dataf[3] = data2[2];
							dataf[4] = data2[3];
							dataf[5] = data2[4];
							dataf[6] = data2[5];
							dataf[7] = data2[6];
						}
					}
				}
			}
		}
	}
	else if(tagDetected)
	{
		while(!(Serial.available() > 0));
		ser = Serial.read();
		if(ser == NFC_TAGQUERY)
			Serial.print(tagType);
		else if(ser == NFC_TAGQUERY_UID)
		{
			Serial.print((char) dataf[0]);
			Serial.print((char) dataf[1]);
			Serial.print((char) dataf[2]);
			Serial.print((char) dataf[3]);
			Serial.print((char) dataf[4]);
			Serial.print((char) dataf[5]);
			if(tagType == NFC_TYPE_STUDENT)
			{
				Serial.print((char) dataf[6]);
				Serial.print((char) dataf[7]);
			}
		}
		else
			Serial.print(SERIAL_ERROR);
	}
	else
	{
		if(Serial.available() > 0)
			if(Serial.read() == NFC_TAGQUERY)
				Serial.print(NFC_NOTAG);
	}
}

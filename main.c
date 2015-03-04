#include <util/delay.h>
#include <avr/io.h>
#include "PN532.h"
#include <PN532_SPI/PN532_SPI.h>

#define	MODEL_QUERY			0x80
#define	SERIAL_ERROR		0x55
#define	NFC_TAGQUERY		0x82
#define	NFC_TAGQUERY_UID	0x84
#define	NFC_ARDUINO			0x02
#define	NFC_NOTAG			0x81
#define	NFC_TYPE_PROFESSOR	0x04
#define	NFC_TYPE_STUDENT	0x01
#define	CPU_FREQ			16000000L

boolean	tagDetected;
boolean	tagType;

PN532_SPI pn532spi(SPI, 10);
PN532 nfc(pn532spi);

void init_serial(int speed)
{
	UBRR0 = CPU_FREQ/(((unsigned long int)speed)<<4)-1;
	UCSR0B = (1<<TXEN0 | 1<<RXEN0);
	UCSR0C = (1<<UCSZ01 | 1<<UCSZ00);
	UCSR0A &= ~(1 << U2X0);
}

void send_serial(unsigned char c)
{
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
}

unsigned char get_serial(void)
{
	loop_until_bit_is_set(UCSR0A, RXC0);
	return UDR0;
}


void setup(void)
{
	int ser;

	init_serial(9600);

	send_serial('1');
	nfc.begin();

	send_serial('2');
	uint32_t versiondata = nfc.getFirmwareVersion();
	if(!versiondata)
	{
		send_serial(SERIAL_ERROR);
		while (1);
	}  

	send_serial('3');
	nfc.SAMConfig();
  
	tagDetected = false;

	do
	{
		ser = get_serial();
		if(ser == MODEL_QUERY)
			send_serial(NFC_ARDUINO);
		else
			send_serial(SERIAL_ERROR);
	}while(ser != MODEL_QUERY);
}


void loop(void)
{
	uint8_t success;
	uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
	uint8_t uidLength;
	uint8_t data1[16];
	uint8_t data2[16];
        char  professorTag[7];
        char  studentTag[9];
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
							professorTag[0] = (char) 48 + data1[4];
							professorTag[1] = (char) 48 + data1[5];
							professorTag[2] = (char) 48 + data1[6];
							professorTag[3] = (char) 48 + data1[7];
							professorTag[4] = (char) 48 + data1[8];
							professorTag[5] = (char) 48 + data1[9];
						}
						else
						{
							tagType = NFC_TYPE_STUDENT;
							studentTag[0] = (char) 48 + data1[15];
							studentTag[1] = (char) 48 + data2[0];
							studentTag[2] = (char) 48 + data2[1];
							studentTag[3] = (char) 48 + data2[2];
							studentTag[4] = (char) 48 + data2[3];
							studentTag[5] = (char) 48 + data2[4];
							studentTag[6] = (char) 48 + data2[5];
							studentTag[7] = (char) 48 + data2[6];
						}
					}
				}
			}
		}
	}
	else if(tagDetected)
	{
		ser = get_serial();
		if(ser == NFC_TAGQUERY)
			send_serial(tagType);
		else if(ser == NFC_TAGQUERY_UID)
		{
                    if(tagType == NFC_TYPE_STUDENT)
                    {/*
                        Serial.print((char) studentTag[0]);
						Serial.print((char) studentTag[1]);
                        Serial.print((char) studentTag[2]);
						Serial.print((char) studentTag[3]);
                        Serial.print((char) studentTag[4]);
						Serial.print((char) studentTag[5]);
                        Serial.print((char) studentTag[6]);
						Serial.print((char) studentTag[7]);*/
                        send_serial(0x10);
						send_serial(0x20);
                        send_serial(0x30);
						send_serial(0x40);
                        send_serial(0x50);
						send_serial(0x60);
	                    send_serial(0x70);
						send_serial(0x80);
                    }
                    else
                    {
                        send_serial(professorTag[0]);
                    }
		    tagDetected = false;
		}
		else
			send_serial((char) SERIAL_ERROR);
	}
	else
	{
		if(get_serial() == NFC_TAGQUERY)
			send_serial(NFC_NOTAG);
	}
}

int main(void)
{
	setup();

	while(1)
		send_serial('Y');
		//loop();

	return 0;
}

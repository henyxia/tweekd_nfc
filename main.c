#include <util/delay.h>
#include <avr/io.h>
#include "PN532.h"
#include <PN532_SPI/PN532_SPI.h>

#define __SCREEN_DEBUG__

#ifdef __SCREEN_DEBUG__
#define	MODEL_QUERY			'A'
#define	SERIAL_ERROR		'G'
#define	NFC_TAGQUERY		'C'
#define	NFC_TAGQUERY_UID	'H'
#define	NFC_ARDUINO			'B'
#define	NFC_NOTAG			'D'
#define	NFC_TYPE_PROFESSOR	'E'
#define	NFC_TYPE_STUDENT	'F'
#else
#define	MODEL_QUERY			0x80
#define	SERIAL_ERROR		0x55
#define	NFC_TAGQUERY		0x82
#define	NFC_TAGQUERY_UID	0x84
#define	NFC_ARDUINO			0x02
#define	NFC_NOTAG			0x81
#define	NFC_TYPE_PROFESSOR	0x04
#define	NFC_TYPE_STUDENT	0x01
#endif

#define	CPU_FREQ			16000000L
#define	SERIAL_SPEED		19200

boolean	tagDetected = false;
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

	init_serial(SERIAL_SPEED);

	nfc.begin();

	uint32_t versiondata = nfc.getFirmwareVersion();
	if(!versiondata)
	{
		send_serial(SERIAL_ERROR);
		while (1);
	}  

	nfc.SAMConfig();
  
	do
	{
		ser = get_serial();
		if(ser == MODEL_QUERY)
			send_serial(NFC_ARDUINO);
		else
			send_serial(SERIAL_ERROR);
	}while(ser != MODEL_QUERY);
}

uint8_t success;
uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
uint8_t uidLength;
uint8_t data1[16];
uint8_t data2[16];
char  professorTag[7];
char  studentTag[9] = {'0', '1', '2', '3', '4', '5', '6', '7'};
int ser;

void loop(void)
{
    
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

				//_delay_ms(50);

				if(success)
				{
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
							//send_serial('M');
							tagType = NFC_TYPE_PROFESSOR;
							professorTag[0] = data1[4];
							professorTag[1] = data1[5];
							professorTag[2] = data1[6];
							professorTag[3] = data1[7];
							professorTag[4] = data1[8];
							professorTag[5] = data1[9];
						}
						else
						{
							//send_serial('N');
							tagType = NFC_TYPE_STUDENT;
							studentTag[0] = data1[15];
							studentTag[1] = data2[0];
							studentTag[2] = data2[1];
							studentTag[3] = data2[2];
							studentTag[4] = data2[3];
							studentTag[5] = data2[4];
							studentTag[6] = data2[5];
							studentTag[7] = data2[6];
						}
					}
					//else
						//send_serial('L');
				}
				//else
					//send_serial('K');
			}
			//else
				//send_serial('J');
		}
		//else
			//send_serial('I');
	}
	else if(tagDetected)
	{
		ser = get_serial();
		if(ser == NFC_TAGQUERY)
			send_serial(tagType);
		else if(ser == NFC_TAGQUERY_UID)
		{
                    if(tagType == NFC_TYPE_STUDENT)
                    {
                        send_serial(studentTag[0]);
						_delay_ms(50);
						send_serial(studentTag[1]);
						_delay_ms(50);
                        send_serial(studentTag[2]);
						_delay_ms(50);
						send_serial(studentTag[3]);
						_delay_ms(50);
                        send_serial(studentTag[4]);
						_delay_ms(50);
						send_serial(studentTag[5]);
						_delay_ms(50);
	                    send_serial(studentTag[6]);
						_delay_ms(50);
						send_serial(studentTag[7]);
						_delay_ms(50);
                    }
                    else
                    {
                        send_serial(professorTag[0]);
                        send_serial(professorTag[1]);
                        send_serial(professorTag[2]);
                        send_serial(professorTag[3]);
                        send_serial(professorTag[4]);
                        send_serial(professorTag[5]);
                    }
		    tagDetected = false;
			_delay_ms(5000);
		}
		else
			send_serial(SERIAL_ERROR);
	}
	else
	{
		if(get_serial() == NFC_TAGQUERY)
			send_serial(NFC_NOTAG);
		else
			send_serial(SERIAL_ERROR);
	}
}

int main(void)
{
	setup();

	while(1)
		loop();

	return 0;
}

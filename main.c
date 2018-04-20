#define TTYNAME	"/dev/ttyAMA0"
#define OUTDIR 	"/tmp/aq/"
#define OUTFILE	"aq.log"

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <stdint.h>

struct PMS5003ST_DATA
{
	uint16_t	START_SEQ;
	uint16_t 	FRAME_SIZE;
	uint16_t	INDUSTRY_PM010;
	uint16_t	INDUSTRY_PM025;
	uint16_t	INDUSTRY_PM100;
	uint16_t	HOME_PM010;
	uint16_t	HOME_PM025;
	uint16_t	HOME_PM100;
	uint16_t	u003;
	uint16_t	u005;
	uint16_t	u010;
	uint16_t	u025;
	uint16_t	u050;
	uint16_t	u100;
	uint16_t	VOC;	//포름알데히드 
	uint16_t	TEMP;
	uint16_t	MOSI;	//습도
	uint16_t	RESERVED;	//..
	uint8_t	version;
	uint8_t	errorCode;
	uint16_t	CHECKSUM;
};

uint16_t transEndian(uint16_t source)
{
	uint16_t tempBuffer = (source >>8)&0xff;
	tempBuffer+=(source&0xff)<<8;
	return tempBuffer;
}

#define FINAL_STAGE	32
#define LOCK_FILE	"/var/lock/monitoringAIR.lock"
int main (void)
{
	//Locking.
	int lockFD = open(LOCK_FILE, O_CREAT|O_EXCL, 0777);
	if (lockFD<0)
	{
		printf("Content-type: text/html\n\nPENDED PROCESS!");
		return lockFD;
	}
	int ttyFD;
	ttyFD = open(TTYNAME, O_RDWR);
	if (ttyFD<0)
	{
		printf ("Content-type: text/html\n\nTTY OPEN ERROR! (%d):\r\n",ttyFD);
		return ttyFD;
	}
	//system("mkdir -p "OUTDIR);
	struct termios serOption;
	memset(&serOption,0,sizeof(struct termios));
	serOption.c_cflag=B9600|CS8|CREAD|CLOCAL;
	serOption.c_iflag=IGNPAR;
	serOption.c_cc[VMIN] = 1;
	tcflush(ttyFD,TCIFLUSH);
	tcsetattr(ttyFD, TCSANOW, &serOption);

	uint32_t pointer = 0;
	uint8_t  ifPreCond = 0;
	uint8_t  ifNextValid = 0;
	
	uint8_t  receivedBuffer[100]={0,};
	uint32_t haveToRead = 0;
	uint32_t frameLength = 0;
	uint32_t checksum = 0;
	while (1)
	{
		//HOW TO BREAK?
		uint8_t oneByte=0;
		read(ttyFD, &oneByte, 1);
		//printf("ReadData:0x%x\r\n",oneByte);
		
		if ((ifPreCond == 0) && (oneByte == 0x42))
		{
			ifPreCond = 1;
			ifNextValid = 0;
			receivedBuffer[0] = oneByte;
			checksum = oneByte;
			continue;
		}
		else if (oneByte == 0x4d && ifPreCond == 1) 
		{
			ifPreCond = 2;
			ifNextValid = 1;
			receivedBuffer[1] = oneByte;
			pointer = 2;
			checksum += oneByte;
			continue;
		}
		
		if (ifNextValid == 1)
		{
			receivedBuffer[pointer]=oneByte;
			switch(pointer)
			{
				case 2: frameLength = oneByte; break;
				case 3: frameLength = (frameLength<<8)+oneByte; break;

			}
			if (	(pointer>3) && (pointer >= frameLength+2) )
			{
				//Bypass Checksum
			}
			else
			{
				checksum+=oneByte;
			}
			pointer++;
			//printf ("Pointer:%d, oneBuffer:%d, checksum:%d\r\n",pointer,oneByte,checksum);
			
			
			if (pointer >= sizeof(receivedBuffer))
			{	//CommError.
				pointer=0;
				ifNextValid = 0;
				ifPreCond = 0;
			}
			if (pointer == frameLength+4)
			{
				struct PMS5003ST_DATA * dataPtr = 0;
				dataPtr = &receivedBuffer;

				uint32_t receivedChecksum = transEndian(dataPtr->CHECKSUM);
				//End of pAcket
				//printf ("Drop The Packet!\r\n");
				//checkTheChecksum
				//printf ("CalculatedChecksum: %d\r\n",checksum);
				//printf ("receivedChecksum:%d\r\n",receivedChecksum);
				if (checksum!=receivedChecksum)
				{
					//ThisPacket is invalid
					printf("Content-type: text/html\n\n INVALID.. WHY?<br>\n");
				}
				else
				{
					//This Packet is valid
					//FILE * FP;
					//FP=fopen(OUTDIR""OUTFILE,"wt");
					printf("Content-type: text/html\n\n");
					printf("<meta charset=\"utf-8\">\n\n");
					printf("<font size=\"6\">\n");
					printf("PM1.0지수:");
					if (transEndian(dataPtr->HOME_PM010) < 50)
						printf ("<font color=GREEN>");
					else if (transEndian(dataPtr->HOME_PM010) < 100)
						printf ("<font color=ORANGE>");
					else
						printf ("<font color=RED>");
					printf("<b>%d</b><br>",transEndian(dataPtr->HOME_PM010));
					printf("</font>\n");
					printf("PM2.5지수:");
					if (transEndian(dataPtr->HOME_PM025) < 50)
						printf("<font color=GREEN>");
					else if(transEndian(dataPtr->HOME_PM025) < 100)
						printf("<font color=ORANGE>");
					else 
						printf("<font color=RED>");
					printf("<b>%d</b><br>",transEndian(dataPtr->HOME_PM025));
					printf("</font>\n");
					printf("PM100지수:");
					if (transEndian(dataPtr->HOME_PM100) < 50)
						printf("<font color=GREEN>");
					else if (transEndian(dataPtr->HOME_PM100) < 100)
						printf("<font color=ORANGE>");
					else
						printf("<font color=RED>");

					printf("<b>%d</b><br>",transEndian(dataPtr->HOME_PM100));
					printf("</font>\n");
					/*printf("세부입자정보:%d, %d, %d, %d, %d, %d<br>\n",
							transEndian(dataPtr->u003),
							transEndian(dataPtr->u005),
							transEndian(dataPtr->u010),
							transEndian(dataPtr->u025),
							transEndian(dataPtr->u050),
							transEndian(dataPtr->u100));
					*/		

					printf("포름알데히드:%4.3f (mg/m^3)<br>\n",transEndian(dataPtr->VOC)/1000.0);
					printf("현재 온도:%3.1f<br>\n",transEndian(dataPtr->TEMP)/10.0);
					printf("현재 습도:%3.1f<br>\n",transEndian(dataPtr->MOSI)/10.0);
					printf("</font>");
					break;
					//close(FP);
					//printf("\r\n\r\n");
				}
				ifNextValid = 0;
				ifPreCond = 0;
				pointer = 0;
			}
		}


	}
	close (ttyFD);
	close (lockFD);
	remove(LOCK_FILE);
}

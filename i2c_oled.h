#ifndef __I2C__OLED__H
#define __I2C__OLED__H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h> // sleep
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "ascii2.h"

int g_fdoled;
char *g_pstroledDev="/dev/i2c-0";
uint8_t g_u8oledAddr=0x78>>1;
enum oled_sig{   //---command or data
oled_SIG_CMD,
oled_SIG_DAT
};
struct flock g_i2cFdReadLock;
struct flock g_i2cFdWriteLock;

//----timer
struct itimerval g_tmval,g_otmval;

//-------functions----
int intFcntlOp(int fd, int cmd, int type, off_t offset, int whence, off_t len);





//---------------------

void initOledTimer(void)
{
   g_tmval.it_value.tv_sec=1;
   g_tmval.it_value.tv_usec=0;
   g_tmval.it_interval.tv_sec=1;
   g_tmval.it_interval.tv_usec=0;

   setitimer(ITIMER_REAL,&g_tmval,&g_otmval);

}



/*--------------send command to oled---------------
  send DATA or COMMAND to oled 
----------------------------------------------*/
void sendDatCmdoled(enum oled_sig datcmd,uint8_t val) {
    int ret;
    uint8_t sig;

    if(datcmd == oled_SIG_CMD)
	sig=0x00; //----send command
    else
	sig=0x40; //--send data

    struct i2c_rdwr_ioctl_data i2c_iodata;

    i2c_iodata.nmsgs=1;
    i2c_iodata.msgs=(struct i2c_msg*)malloc(i2c_iodata.nmsgs*sizeof(struct i2c_msg));
    i2c_iodata.msgs[0].len=2;
    i2c_iodata.msgs[0].addr=g_u8oledAddr;
    i2c_iodata.msgs[0].flags=0; //write
    i2c_iodata.msgs[0].buf=(unsigned char*)malloc(2);
    i2c_iodata.msgs[0].buf[0]=sig; // 0x00 for Command, 0x40 for data
    i2c_iodata.msgs[0].buf[1]=val; 

    ioctl(g_fdoled,I2C_TIMEOUT,2);
    ioctl(g_fdoled,I2C_RETRIES,1);

    ret=ioctl(g_fdoled,I2C_RDWR,(unsigned long)&i2c_iodata);
    if(ret<0)
    {
	printf("i2c ioctl read error!\n");
    }

    free(i2c_iodata.msgs);
    free(i2c_iodata.msgs[0].buf);
}

/*------ send command to oled ---------*/
void sendCmdOled(uint8_t cmd)
{
   sendDatCmdoled(oled_SIG_CMD,cmd);
}

/*------ send  data to oled ---------*/
void sendDatOled(uint8_t dat)
{
   sendDatCmdoled(oled_SIG_DAT,dat);
}



/*----- open i2c slave and init oled with default param-----*/
void initOledDefault(void)
{
  int fret;
  struct flock lock;

  if((g_fdoled=open(g_pstroledDev,O_RDWR))<0)
  {
	perror("fail to open i2c bus");
        exit(1);
  }
  else
   	printf("Open i2c bus successfully!\n");

  //------ try to lock file
  intFcntlOp(g_fdoled,F_SETLK, F_WRLCK, 0, SEEK_SET,0);//write lock
//  intFcntlOp(g_fdoled,F_SETLK, F_RDLCK, 0, SEEK_SET,0);//read lock
  printf("I2C fd lock operation finished.\n");


  sendCmdOled(0xAE); //display off
  //-----------------------
  sendCmdOled(0x20);  //set memory addressing mode
  sendCmdOled(0x22); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)
  //-----------------------
  sendCmdOled(0xb0);  // 0xb0~b7 set page start address for page addressing mode
  //-----------------------
  sendCmdOled(0xc8);// set COM Output scan direction  0xc0-- COM0 to COM[N-1], 0xc8-- COM[N-1] to COM0
  //-----------------------
  sendCmdOled(0x0f);//2); //0x00~0F, set lower column start address for page addressing mode
  //-----------------------
  sendCmdOled(0x10); // 0x10~1F, set higher column start address for page addressing mode
  //-----------------------
  sendCmdOled(0x40); //40~7F set display start line 0x01(5:0)
  //--------------------
  sendCmdOled(0x81); // contrast control
  sendCmdOled(0x7f); //contrast value RESET=7Fh 
  //--------------------
  sendCmdOled(0xa1); //set segment remap a0h--column 0 is mapped to SEG0, a1h--column 127 is mapped to SEG0
  // ------------------
  sendCmdOled(0xa6); //normal / reverse a6h--noraml, 1 in RAM displays; a7h--inverse, 0 in RAM displays.
  //------------------
  sendCmdOled(0xa8); //multiplex ratio
  sendCmdOled(0x3F); // 16MUX to 64MUX RESET=111111b, 0x1F for 128x32; 0x3F for 128x64
  //----------------
  sendCmdOled(0xa4);// A4h-- resume to RAM content display(RESET), A5h--entire display ON.
  //----------------
  sendCmdOled(0xd3);// set display offset, set vertical shift by COM from 0d~63d
  sendCmdOled(0x00);
  //----------------
  sendCmdOled(0xd5); // set desplay clock divide,
  sendCmdOled(0xf0);// [7:4] osc. freq; [3:0] divide ratio, RESET is 00001000B 
  //----------------
  sendCmdOled(0xd9);// set pre-charge period 
  sendCmdOled(0x22);//[3:0] phase 1 period of up to 15 DCLK [7:4] phase 2 period of up to 15 DCLK 
  //----------------
  sendCmdOled(0xda);//--set COM pins hardware configuration
  sendCmdOled(0x12);//[5:4]  0x02 for 128x32 0x12 for 128x64
  //----------------
  sendCmdOled(0xdb);//set Vcomh deselect level
  sendCmdOled(0x20);
  //=-------------- 
  sendCmdOled(0x8d);// charge pump setting 
  sendCmdOled(0x14);// [2]=0 disable charge pump, [2]=1 enbale charge pump
  //---------------
  sendCmdOled(0xaf);// AE, display off(sleep mode), AF, display on in normal mode.

}


void fillOledDat(uint8_t dat)
{
    uint8_t i,j,k;
    //--sed page addressing mode---
    sendCmdOled(0x20);  //set memory addressing mode
    sendCmdOled(0x22); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)

	for(i=0;i<8;i++)
	{
		sendCmdOled(0xb0+i); //0xB0~B7, page0-7
		sendCmdOled(0x00); //00~0f,low column start address  ?? no use !!
		sendCmdOled(0x10); //10~1f, ???high column start address
		for(j=0;j<128;j++) // write to 128 segs, 1 byte each seg.
		{
			sendDatOled(dat);
		}

	}
}

//----XXXXX to use clearOledV() instead !!!!
void clearOled(void)
{
    int i,j;
 
    //--sed page addressing mode---
    sendCmdOled(0x20);  //set memory addressing mode
    sendCmdOled(0x22); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)

    for(i=0;i<8;i++)
    {
	  sendCmdOled(0xb0+i);  // 0xb0~b7 set page start address for page addressing mode
	  sendCmdOled(0x00); // low column start address
	  sendCmdOled(0x10); //0x10~1f high column start address,use 0x10 if no limits.
	  for(j=0;j<128;j++)
		sendDatOled(0x00); //--clear  GRAM
     }

}

/*----------------------------------------------------
  draw Ascii symbol on Oled
  start_column: 0~7  (128x64)
  sart_row:  0~15;!!!!!!
------------------------------------------------------*/
void  drawOledAscii16x8(uint8_t start_row, uint8_t start_column,unsigned char c)
{
	int i,j;

	//----set mode---
	//-----set as vertical addressing mode -----
        sendCmdOled(0x20);  //set memory addressing mode
        sendCmdOled(0x01); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)
	//---set column addr. for horizontal mode
	sendCmdOled(0x21);
	sendCmdOled(start_column);//column start
	sendCmdOled(start_column+7);//column end, !!!! 8x16 for one line only!!!
	//---set page addr. 
	sendCmdOled(0x22);
	sendCmdOled(start_row);// 0-7 start page
	sendCmdOled(start_row+1);// 2 rows. 2x8=16  !!!! for one line only!!!!

	for(j=0;j<16;j++)
		sendDatOled(ascii_8x16[(c-0x20)*16+j]);


}

void  drawOledStr16x8(uint8_t start_row, uint8_t start_column,const char* pstr)
{
    int k;
    int len=strlen(pstr);
  
    for(k=0;k<len;k++)
        drawOledAscii16x8(start_row,start_column+8*k,*(pstr+k));

}


//--------- horizontal mode -----  FAIL !!!!
void  drawOledAsciiHRC(uint8_t start_row, uint8_t start_column,unsigned char c)
{
	int i,j;

	sendCmdOled(0xb0+start_row); //0xB0~B7, page0-7
	sendCmdOled(0x00+start_column); //00~0f,low column start address  ?? no use !!
	sendCmdOled(0x1f); //10~1f, ???high column start address

	for(j=0;j<16;j++) // write to 128 segs, 1 byte each seg.
	{
		sendDatOled(ascii_8x16[(c-0x20)*16+j]);
	}

}


//----- routine for SIGALRM ---
void sigHndlOledTimer(int signo)
{
    drawOledStr16x8(6,0,"               ");
    usleep(500000);
    drawOledStr16x8(6,0,"    --test--   ");  

}

//------file lock/unlock operation ----

int intFcntlOp(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{

    int ret,fcret;
    struct flock lock;


     if((fcret=fcntl(g_fdoled,F_GETFL,0))<0)
     {
 	perror("fcntl to get lock");
 	exit(1);
     }

      switch(fcret & O_ACCMODE)
      {
	case O_RDONLY:
		printf("Read only..\n");break;
	case O_WRONLY:	
		printf("Write only..\n");break;
	case O_RDWR:	
		printf("Read and Write ..\n");break;
	default:
		printf("File is not valid..\n");
       }





    //----init lock with value, otherwise "Invalid argument" error
    lock.l_type=F_WRLCK; //--check whether F_WRLCK is lockable
    lock.l_start=0;
    lock.l_whence=SEEK_SET;
    lock.l_len=0;
     //---- check lock ---
    if(fcntl(g_fdoled,F_GETLK,&lock)<0) //--lock return as UNLCK if is applicable, or it returns file's current lock.
    {
 	perror("fcntl to get lock");
 	exit(1);
     }

     //-------------UNAPPLICABLE !!!!!
     printf("lock.l_type: 0x%x\n",lock.l_type);
     printf("F_WRLCK: 0x%x\n",F_WRLCK);

    if((lock.l_type==F_WRLCK) || (lock.l_type==F_RDLCK) )
    {
	printf("File is locked!\n");
  	exit(1);
     }

    //-----reset value
    lock.l_type=type;
    lock.l_start=offset;
    lock.l_whence=whence;
    lock.l_len=len;
    //-----fcntl lock
    if((ret=fcntl(fd,cmd,&lock))<0)
    {
	printf("fcntl operation error!\n");
	exit(1);
    }
    printf("fcntl ret=%d\n",ret);
    return ret;
}

/*-------------------------------------
clear oled with vertical addressing mode
--------------------------------------*/
void  clearOledV(void)
{
	int i,j;

	//----set mode---
	//-----set as vertical addressing mode -----
        sendCmdOled(0x20);  //set memory addressing mode
        sendCmdOled(0x01); //[1:0]=00b-horizontal 01b-veritacal 10b-page addressing mode(RESET)
	//---set column addr. for horizontal mode
	sendCmdOled(0x21);
	sendCmdOled(0);//column start
	sendCmdOled(127);//column end, !!!! 8x16 for one line only!!!
	//---set page addr. 
	sendCmdOled(0x22);
	sendCmdOled(0);// 0-7 start page
	sendCmdOled(7);// 2 rows. 2x8=16  !!!! for one line only!!!!

	for(j=0;j<8*128;j++)
		sendDatOled(0x00);
}




#endif

/*-----------------------  touch_home.c ------------------------------
1. Only first byte read from XPT is meaningful !!!??

2. Keep enough time-gap for XPT to prepare data in each read-session,
usleep 3000us seems OK, or the data will be too scattered.

3. Hold LCD-Touch module carefully, it may influence touch-read data.

4. TOUCH Y edge doesn't have good homogeneous property along X,
   try to draw a horizontal line and check visually, it will bend.

5. if sx0=sy0=0, it means that last TOUCH coordinate is invalid or pen-up.

6. Linear piont to piont mapping from TOUCH to LCD. There are
   unmapped points on LCD.

7. point to an oval area mapping form TOUCH to LCD.


TODO:
0. Cancel extern vars in all head file, add xx_get_xxVAR() functions.
1. Screen sleep
2. LCD_SIZE_X,Y to be cancelled. use FB parameter instead.
3. home page refresh button... 3s pressing test.
4. apply mutli-process for separative jobs: reading-touch-pad, CLOCK,texting,... etc.
5. systme()... sh -c ...
6. btn-press and btn-release signal
7. To read FBDE vinfo to get all screen/fb parameters as in fblines.c, it's improper in other source files.

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <signal.h>
#include "color.h"
#include "spi.h"
#include "fblines.h"
#include "egi.h"
#include "egi_timer.h"
#include "egi_obj.h"
#include "xpt2046.h"
#include "bmpjpg.h"
//#include "dict.h"
#include "symbol.h"


/*  ---------------------  MAIN  ---------------------  */
int main(void)
{
	int i,j;
	int index;
	int ret;
	uint16_t sx,sy;  //current TOUCH point coordinate, it's a LCD screen coordinates derived from TOUCH coordinate.

	int delt=5; /* incremental value*/
	//char str_syscmd[100];
	//char strf[100];
	int disk_on=0;
	int radio_on=0;
	int kr=0;

	uint16_t *buf;
	buf=(uint16_t *)malloc(320*240*sizeof(uint16_t));



	/* ------------ NOTE ebox test ------------------ */
	struct egi_data_txt note_txt={0};
	/* init txtbox data: offset(10,10) 2_lines, 510bytes per txt line,font, font_color */
	if( egi_init_data_txt(&note_txt, 5, 5, 2, 510, &sympg_testfont, WEGI_COLOR_BLACK) ==NULL ) {
		printf("init NOTE data txt fail!\n"); 
		exit(1);
	 }
	struct egi_element_box ebox_note=
	{
		.movable = true,
		.type = type_txt,
		.egi_data =(void *) &note_txt,
		.height = 60, /* two line */
		.width = 230,
		.prmcolor = WEGI_COLOR_GRAY,/* <0, transparent */
		.x0= 5,  //5
		.y0= 80, //320-80,
		.tag="note pad",
	};

	/* ------------ CLOCK ebox test ------------------- */
	struct egi_data_txt clock_txt={0};
	/* init txtbox data: offset(x,y) 1_lines, 480bytes per txt line,font, font_color */
	if( egi_init_data_txt(&clock_txt, 0, 0, 1, 120, &sympg_numbfont,WEGI_COLOR_BROWN) ==NULL ) {
		printf("init CLOCK data txt fail!\n"); exit(1);
	 }
	struct egi_element_box ebox_clock=
	{
		.movable = true,
		.type = type_txt,
		.egi_data =(void *) &clock_txt,
		.height = 20, /* ebox height */
		.width = 120,
		.prmcolor = EGI_NOPRIM_COLOR, /*-1, if<0,transparent */
		.x0= 60,
		.y0= 5,//320-38,
		.frame=-1, /* <0, no frame */
		.tag="timer txt",
	};


#if 0
	/* ------------ MEMO ebox test ------------------ */
	struct egi_data_txt memo_txt={0};
	/* init txtbox data: txt offset(5,5) to box, 12_lines, 24bytes char per line, font, font_color */
	if( egi_init_data_txt(&memo_txt, 5, 5, 12, 24, &sympg_testfont, WEGI_COLOR_BLACK) == NULL ) {
		printf("init MEMO data txt fail!\n"); exit(1);
	}
	/* indicate a txt file */
	memo_txt.fpath="/home/memo.txt";
	//memo_txt.foff=0;
	struct egi_element_box ebox_memo=
	{
		.movable=true,
		.type = type_txt,
		.egi_data =(void *)&memo_txt, /* try &note_txt.....you may use other txt data  */
		.height = 320, /*box height, one line, will be ajusted according to numb of lines */
		.width = 240,
		.prmcolor = WEGI_COLOR_GRAY, //EGI_NOPRIM_COLOR, //WEGI_COLOR_ORANGE,
		.x0= 12,
		.y0= 0, // 25 - 320,
		.frame=-1, //no frame
		.tag="memo stick",
	};

#endif

	/* ------------   home_button eboxes definition  ------------------ */
	struct egi_element_box  ebox_buttons[9]={0};
	struct egi_data_btn home_btns[9]={0};
	for(i=0;i<3;i++) /* row of icon img */
	{
		for(j=0;j<3;j++) /* column of icon img */
		{
			home_btns[3*i+j].shape=square;
			home_btns[3*i+j].id=3*i+j;
			home_btns[3*i+j].icon=&sympg_icon;
			home_btns[3*i+j].icon_code=3*i+j;	/* symbol code number */
			/* hook to ebox model */
			ebox_buttons[3*i+j].y0=105+(15+60)*i;
			ebox_buttons[3*i+j].x0=15+(15+60)*j;
			ebox_buttons[3*i+j].type=type_button;
			ebox_buttons[3*i+j].egi_data=(void *)(home_btns+3*i+j);
			sprintf(ebox_buttons[3*i+j].tag,"button_%d",3*i+j);
		}
	}

#if 0 /* test ----- egi txtbox read file ---------- */
	 ret=egi_txtbox_readfile(&ebox_memo, "/tmp/memo.txt");
	 printf("ret=egi_txtbox_readfile()=%d\n",ret);
	 exit(1);
#endif

	/* --- open spi dev --- */
	SPI_Open();

	/* --- prepare fb device --- */
        gv_fb_dev.fdfd=-1;
        init_dev(&gv_fb_dev);


	/* --- clear screen with BLACK --- */
#if 0
	clear_screen(&gv_fb_dev,(0<<11|0<<5|0));
#endif

	/* --- load screen paper --- */
	show_jpg("home.jpg",&gv_fb_dev,0,0,0); /*black on*/


	/* --------- test image rotate ----------- */
#if 1
        /* copy fb image to buf */
	int centx=120;
	int centy=120;
	int sq=141;
        fb_cpyto_buf(&gv_fb_dev, centx-sq/2, centy-sq/2, centx+sq/2, centy+sq/2, buf);
	/* for image rotation */
	struct egi_point_coord	*SQMat; /* the map matrix,  101=2*50+1 */
	SQMat=malloc(141*141*sizeof(struct egi_point_coord));
	memset(SQMat,0,sizeof(SQMat));
	struct egi_point_coord  centxy={centx,centy}; /* center of rotation */
	struct egi_point_coord  x0y0={centx-sq/2,centy-sq/2};
#endif

#if 0
	while(1)
	{
		i++;
		/* get rotation map */
		mat_pointrotate_SQMap(101, 5*i, centxy, SQMat);/* side,angle,center, map matrix */
		/* draw rotated image */
		fb_drawimg_SQMap(101, x0y0, buf, SQMat); /* side,center,image buf, map matrix */
	}
	exit(1)
#endif



	/* --- print and display symbols --- */
#if 0
	for(i=0;i<10;i++)
		dict_writeFB_symb20x15(&gv_fb_dev,1,(30<<11|45<<5|10),i,30+i*15,320-40);
#endif

	/*------------------ Load Symbols ------------------*/
	/* load testfont */
	if(symbol_load_page(&sympg_testfont)==NULL)
		exit(-2);
	/* load numbfont */
	if(symbol_load_page(&sympg_numbfont)==NULL)
		exit(-2);
	/* load icons */
	if(symbol_load_page(&sympg_icon)==NULL)
		exit(-2);


	/* --------- test:  print all symbols in the page --------*/
#if 0
	for(i=32;i<127;i++)
	{
		symbol_print_symbol(&sympg_testfont,i,0xffff);
		//getchar();
	}
#endif
#if 0
	for(i=48;i<58;i++)
		symbol_print_symbol(&sympg_numbfont,i,0x0);
#endif



#if 0 /* ----  test circle ----------*/
	fbset_color(WEGI_COLOR_OCEAN);
	draw_filled_circle(&gv_fb_dev,120,160,90);
	fbset_color(0);
	draw_circle(&gv_fb_dev,120,160,90);
exit(1);
#endif

	/* ----------- activate txt and note eboxes ---------*/
	/* note:
	   Be careful to activate eboxes in the correct sequence.!!!
	   activate static eboxes first(no bkimg), then mobile ones(with bkimg),
	*/
	/*  buttons  */
	for(i=0;i<9;i++)
		egi_btnbox_activate(ebox_buttons+i);
	/* txt clock */
	egi_txtbox_activate(&ebox_clock); /* no time string here...*/
	egi_txtbox_sleep(&ebox_clock);/* put to sleep */
	/* txt note */
	egi_txtbox_activate(&ebox_note);
        egi_txtbox_sleep(&ebox_note); /* put to sleep */
	//egi_txtbox_activate(&ebox_note);/* wake up */


	/* txt memo */
	//egi_txtbox_activate(&ebox_memo);
	//egi_txtbox_sleep(&ebox_memo);
	egi_obj_txtmemo_init();

	/* ---- set timer for time display ---- */
	tm_settimer(500000);/* set timer interval interval */
	signal(SIGALRM, tm_sigroutine);

	tm_tick_settimer(TM_TICK_INTERVAL);/* set global tick timer */
	signal(SIGALRM, tm_tick_sigroutine);


	/* ----- set default color ----- */
        fbset_color((30<<11)|(10<<5)|10);/* R5-G6-B5 */

	/*  test an array of circles */
#if 0
	for(i=0;i<6;i++)
	{
		if(i==0)fbset_color((0<<11)|(60<<5)|30);
		if(i==1)fbset_color((30<<11)|(60<<5)|0);
		if(i==2)fbset_color((0<<11)|(0<<5)|30);
		if(i==3)fbset_color((30<<11)|(0<<5)|0);
		if(i==4)fbset_color((0<<11)|(60<<5)|0);
		if(i==5)fbset_color((30<<11)|(0<<5)|30);

		draw_filled_circle(&gv_fb_dev,20+i*i*7,70,10+i*4);
	}
#endif

	/* --- copy partial fb mem to buf -----*/
	//fb_cpyto_buf(&gv_fb_dev, 100,0,150,320-1, buf);


/* ===============-------------(((  MAIN LOOP  )))-------------================= */
	while(1)
	{
		/*------ relate with number of touch-read samples -----*/
		//usleep(6000); //3000
		tm_delayms(2);
		/*--------- read XPT to get avg tft-LCD coordinate --------*/
		ret=xpt_getavg_xy(&sx,&sy); /* if fail to get touched tft-LCD xy */
		if(ret == XPT_READ_STATUS_GOING )
		{
			continue; /* continue to loop to finish reading touch data */
		}


		/* -------  put PEN-UP status events here !!!! ------- */
		else if(ret == XPT_READ_STATUS_PENUP )
		{
		  /*  Heavy load task MUST NOT put here ??? */
			/* get hour-min-sec and display */
			tm_get_strtime(tm_strbuf);

/* TODO: if NOTE and MEMO has the same interval value,then the later one will never be performed !!! */
			/* refresh timer NOTE eboxe according to tick */
			if( tm_get_tickcount()%100 == 0 ) /* 30*TM_TICK_INTERVAL(5000us) */
			{
				//printf("tick = %lld\n",tm_get_tickcount());
				if(ebox_note.x0 <=60  ) delt=10;
				if(ebox_note.x0 >=300 ) delt=-10;
				ebox_note.x0 += delt; //85 - (320-60)
				egi_txtbox_refresh(&ebox_note);
			}
			/* refresh MEMO eboxe according to tick */
#if 1
			if( tm_get_tickcount()%3000 == 0 ) /* 1000*TM_TICK_INTERVAL(5ms) */
			{
				//ebox_memo.y0 += 3;
				egi_txtbox_refresh(&ebox_memo);
			}
#endif
			/* -----ONLY if tm changes, update txt and clock */
			if( strcmp(note_txt.txt[1],tm_strbuf) !=0 )
			{
				/* update NOTE ebox txt  */
				strncpy(note_txt.txt[1],tm_strbuf,10);
				/* -----refresh CLOCK ebox---- */
				//wirteFB_str20x15(&gv_fb_dev, 1, (30<<11|45<<5|10), tm_strbuf, 60, 320-38);
				strncpy(clock_txt.txt[0],tm_strbuf,10);
				clock_txt.color += (6<<8 | 4<<5 | 2 );
				egi_txtbox_refresh(&ebox_clock);
			}

			/* get year-mon-day and display */
			tm_get_strday(tm_strbuf);
//			symbol_string_writeFB(&gv_fb_dev, &sympg_testfont,WEGI_COLOR_SPRINGGREEN,
//					SYM_FONT_DEFAULT_TRANSPCOLOR,32,90,tm_strbuf);//(32,90,12,2)
			/* copy to note_txt */
			strncpy(note_txt.txt[0],tm_strbuf,22);


			/* ----------- test rotation map------------- */
		    	if(disk_on)
		    	{
				kr++;
				/* get rotation map */
				mat_pointrotate_SQMap(sq, 1*kr, centxy, SQMat);/* side,angle,center, map matrix */
				/* draw rotated image */
				fb_drawimg_SQMap(sq, x0y0, buf, SQMat); /* side,center,image buf, map matrix */
		    	}

			continue; /* continue to loop to read touch data */
		}


		else if(ret == XPT_READ_STATUS_COMPLETE)
		{
			printf("--- XPT_READ_STATUS_COMPLETE ---\n");
			/* going on then to check and activate pressed button */

		}

	/* -----------------------  Touch Event Handling  --------------------------*/
		/*---  get index of pressed ebox and activate the button ----*/
	    	index=egi_get_boxindex(sx,sy,ebox_buttons,9);
		printf("get touched box index=%d\n",index);

#if 1
		if(index>=0) /* if get valid index */
		{
			printf("button[%d] pressed!\n",index);
			switch(index)
			{
				case 0: /* use */
					printf("refresh fb now.\n");
					system("/tmp/tmp_app");
					exit(1);
					break;
				case 1:
					break;
				case 2:
					break;
				case 3:
					break;
				case 4: /*--------ON/OFF:  disk play ---------*/
					if(disk_on)
					{
						disk_on=0;
						system("killall mplayer");
					}
					else
					{
						disk_on=1;
						system("mplayer -af volume=9:1 /mmc/friends.mp3 >/dev/null 2>&1 &");
					}
					tm_delayms(300);
					break;
				case 5: /*-------ON/OFF:  memo txt display -------*/
					if(ebox_memo.status!=status_active)
					{
						printf("BEFORE: egi_txtbox_activate(&ebox_memo)\n");
						egi_txtbox_activate(&ebox_memo);
					}
					else if(ebox_memo.status==status_active)
						egi_txtbox_sleep(&ebox_memo);
					tm_delayms(200);
					//for(i=0;i<5;i++)
					//	usleep(800000);
					break;
				case 6: break;
				case 7: /*------ON/OFF:  memo txt refresh -------*/
					egi_txtbox_refresh(&ebox_memo);
					break;
				case 8:
					if(radio_on)
					{
						radio_on=0;
						system("killall mplayer");
					}
					else
					{
						radio_on=1;
						system("/home/eradio.sh");
					}
					tm_delayms(200);
					break;
			}/* switch */
		//}
			//usleep(200000); //this will make touch points scattered.
		}/* end of if(index>=0) */
#endif

	} /* end of while() loop */

	/* release symbol mem page */
	symbol_release_page(&sympg_testfont);

	/* close fb dev */
        munmap(gv_fb_dev.map_fb,gv_fb_dev.screensize);
        close(gv_fb_dev.fdfd);

	/* close spi dev */
	SPI_Close();
	return 0;
}

/*----------------------------------------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

Based on:
          FFmpeg examples in code sources.
				       --- FFmpeg ORG
          dranger.com/ffmpeg/tutorialxx.c
 				       ---  by Martin Bohme
	  muroa.org/?q=node/11
				       ---  by Martin Runge
	  www.xuebuyuan.com/1624253.html
				       ---  by niwenxian


TODO:
0. Only for 1 or 2 channels audio data.
1. Convert audio format AV_SAMPLE_FMT_FLTP(fltp) to AV_SAMPLE_FMT_S16.
   It dosen't work for ACC_LC decoding, lots of float point operations.

2. Can not decode png picture in a mp3 file. ...OK, with some tricks:
   !!! NOTE: Use libs of ffmpeg-2.8.15, except libavcodec.so.56.26.100 of ffmpeg-2.6.9 from openwrt !!!
   However, fail to put logo on mp3 picture by AVFilter descr.

3. FFmpeg PCM 'planar' format means noninterleaved? while 'packed' mean interleaved?
        XXX_FMT_S16P --- packed ???  interleaved
        XXX_FMT_S16  --- planar ???  noninterleaved

4. ffmusic.c and egi_pcm.c only support AV_SAMPLE_FMT_S16(P) now:
        enum AVSampleFormat {     ---  FFMPEG SAMPLE FORMAT  ---
            AV_SAMPLE_FMT_NONE = -1,
            AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
            AV_SAMPLE_FMT_S16,         ///< signed 16 bits
            AV_SAMPLE_FMT_S32,         ///< signed 32 bits
            AV_SAMPLE_FMT_FLT,         ///< float
            AV_SAMPLE_FMT_DBL,         ///< double

            AV_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
            AV_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
            AV_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
            AV_SAMPLE_FMT_FLTP,        ///< float, planar
            AV_SAMPLE_FMT_DBLP,        ///< double, planar

            AV_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
        };

#define AV_CH_LAYOUT_MONO              (AV_CH_FRONT_CENTER)  4
#define AV_CH_LAYOUT_STEREO            (AV_CH_FRONT_LEFT|AV_CH_FRONT_RIGHT) 1+2=3


5. Start ffplay movie with png logo from script /etc/rc.local fails when reboot Openwrt:
	... error log ...
	...
	filter graph args: video_size=160x90:pix_fmt=0:time_base=125/2997:pixel_aspect=45/44
	[Parsed_movie_0 @ 0x9b9c00] Failed to avformat_open_input 'logo.png'
	Error initializing filter 'movie' with args 'logo.png'
	Fail to call avfilter_graph_parse_ptr() to parse fitler graph descriptions.
	joint picture displaying thread ...
	...

NOTE:
1.  A simpley example of opening a media file/stream then decode frames and send RGB data to LCD for display.
    Files without audio stream can also be played, or audio files.
A.  IF input audio(mp3) sample rate is not 44100, then use SWR to covert it.
B.  Assume all output pcm data is nonintervealed type. (except FLTP format ?).
2.  Decode audio frames and playback by alsa PCM.
3.  DivX(DV50) is better than Xdiv. Especially in respect to decoded audio/video synchronization,
    DivX is nearly perfect.
4.  It plays video files smoothly with format of 480*320*24bit FP20, encoded by DivX.
    Decoding speed also depends on AVstream data rate of the file. ( for USB Bridged )
5.  The speed of whole procedure depends on ffmpeg decoding speed, FB wirte speed and LCD display speed.
6.  Please also notice the speed limit of your LCD controller, It's 500M bps for ILI9488???
7.  Cost_time test codes will slow down processing and cause choppy.
8.  Use unstripped ffmpeg libs.
9.  Try to play mp3 with album picture. :)
10. For stream media, avformat_open_input() will fail if key_frame is corrupted.
11. Change AVFilter descr to set clock or cclock, var. transpos_colock is vague.
12. If logo.png is too big....?!!!!
13. The final display windown H/W(row/column pixel numbers) must be multiples of 16 if AVFilter applied,
    and multiples of 2 while software scaler applied.
14. When display a picture, you shall wait for a while after finish
15. Put the subtitle file at the same directory as of the media file, the program will automatically get it
    if it exists, Only 'srt' type subtitle file is supported.



		 	(((  --------  PAGE DIVISION  --------  )))
[Y0-Y29]
{0,0},{240-1, 29}  		--- Head titlebar

[Y30-Y260]
{0,30}, {240-1, 260}  		--- Image/subtitle Displaying Zone
[Y30-Y149]  Image
[Y150-Y260] Sub_displaying

[Y150-Y265]
{0,150}, {240-1, 260} 		--- box area for subtitle display

[Y266-Y319]
{0,266}, {240-1, 320-1}  	--- Buttons


			 (((  --------  Glossary  --------  )))

FPS:		Frame Per Second
PAR:		Pixel Aspect Ratio
SAR:		Sample Aspect Ratio
	     	!!!!BUT, in avcodec.h,  struct AVCodecContext->sample_aspect_ratio is "width of a pixel divided by the height of the pixel"
DAR:		Display Aspect Ratio
PIX_FMT:	pixel format defined in libavutil/pixfmt.h
FFmpeg transpose:
		1. Map W(row) pixels to H(column) pixels , or vise versa.
           	2. For avfilter descr normal 'scale=WxH' (image upright size!!! NOT LCD WxH!!!)
	   	   !!! normal scale= image_W x image_H (image upright size!!!),
 	   	3. If transpose clock or cclock, it shall set to be 'scale=display_H x display_W',
	   	   LCD always maps display_W to row, and display_H to column.
	   	4. Finally, image_H map to display_W if clock/cclock transpose applied.

display_height,display_width:
	   	Mapped to final display WxH as of LCD row_pixel_number x column_pixel_number
	   	(NOT image upright size!!!)


		 	 (((  --------  Data Flow  --------  )))

The data flow of a movie is like this:
  (main)    	FFmpeg video decoding (~10-15ms per frame) ----> pPICBuff
  (thread)  	pPICBuff ----> FB (~xxxms per frame) ---> Display
  (thread)  	display subtitle
  (main)    	FFmpeg audio decoding ---> write to PCM ( ~2-4ms per packet?)


Usage:
	ffplay  media_file or media_address
	ffplay /music/ *.mp3
	ffplay /music/ *.mp3  /video/ *.avi
	ffplay /media/ *

	Well, try this with AUDIO off:
	ffplay http://devimages.apple.com.edgekey.net/streaming/examples/bipbop_4x3/gear2/prog_index.m3u8


Midas Zhou
midaszhou@yahoo.com
-----------------------------------------------------------------------------------------------------------*/
#include <signal.h>
#include <math.h>
#include <libgen.h>
#include <string.h>

#include "egi_common.h"
#include "egi_FTsymbol.h"
#include "sound/egi_pcm.h"
#include "utils/egi_cstring.h"
#include "utils/egi_utils.h"
#include "page_ffmotion.h"
#include "ffmotion_utils.h"
#include "ffmotion.h"

#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libavutil/timestamp.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"

#define LOOP_HOLDON_TIME  1  /* in second, hold_on time after ffplaying a file, especially for a picture.
			      *  set before FAIL_OR_TERM.
			      */
#define CLIP_PLAYTIME 	  3     /* in second, set clip play time */
#define END_PAUSE_TIME    0  /* 2 ?? end pause time */

#define ENABLE_MEDIA_LOOP



/*--------  FFPLAY ENVIRONTMENT SET  ------------
 To set it up first before you call egi_ffplay()
------------------------------------------------*/
FFMOTION_CONTEXT *FFmotion_Ctx=NULL;

/* FB DEV for picture displaying */
FBDEV ff_fb_dev;

int ff_sec_Vduration=0; /* in seconds, multimedia file Video duration */
int ff_sec_Aduration=0; /* in seconds, multimedia file Audio duration */
int ff_sec_Velapsed=0;  /* in seconds, playing time elapsed for Video */
int ff_sec_Aelapsed=0;  /* in seconds, playing time elapsed for Audio */


/*
ffplay() parameters:
 *	0. displaying window offset x0,y0
 *	1. display_width, display_height; (displaying window size, relating to LCD coordinate,map to LCD WxH)
	   will be adjusted.
 *	2. transpose_clock; for avfiler descr, transpose clock or not.
 *	3. out_sample_rate for PCM

Note:
 *	1.
 *	2.
*/



/* Margins than NOT for displaying image, X aligned to screen W, Y alinged to screen H, as of SCREEN COORD. */
static int  scrnMargX_Left=30;    /* 40 for timing bar. ---- for Landscape mode ---- */
static int  scrnMargX_Right=40;   /* 20 for title       */
static int  scrnMargX =30+40;     /* = scrnMargX_Left+scrnMargX_Right */

static int  scrnMargY_Upp=60-5;
static int  scrnMargY_Down=0;
static int  scrnMargY =55;	 /* = scrnMargY_Upp+scrnMargY_Down */

/* Expected Max. display window size, aligned with original LCD screen coord. W/H !!!
 * 1. It will be adjusted according to the actual image size/ratio and useful screen size
 *    (deduct scrnMargX and scrnMargY).
 *				( --- displaying window size processing routine  --- )
 *    1.1 display_width*height ---> fit into (scrnX-scrnMargX)*(scrnY-scrnMargY)---> newdisplay_width*height
 *    1.2 widthOrig*heightOrig ---> fit into new display_width*hegith ---> scwidth*scheight
 *    1.3 Finally re_assign: display_width=scwidth; display_height=scheight
 *
 * 2. If actual image W and H are both smaller, then display_width/h will be ingored.
 * 3. If disable_scale_size=true, then display_width and display_height will be assigned as scrnUseX and scrnUseY
 *    at first.
 *
 *		( --- Landscape mode --- )
 *  	display_width MAX. 240-scrnMargX,
 *	display_height MAX. 320-scrnMargY.
 *
 * 		( --- Portrait  mode --- )
 *	display_width MAX. 240
 *	display_height MAX. 230
 */
static int display_width=  240-70;        /* in LCD row pixels,  */
static int display_height= 320;       /* in LCD column pixels */

/* offset of the show window relating to LCD origin */
static int offx;
static int offy;


/***  ( precondition: image size can fit into scrnUseX*scrnUseY, OR disable_scale_size will be reset to FALSE )
 *  If true:  Original image size will be applied, if precondition is OK.
 *  Note: We can NOT disable SWS, as it converts color format to AV_PIX_FMT_RGB565LE.
 */
static bool disable_scale_size=true;

/* param: ( enable_audio_spectrum ) ( precondition: audio is ON and available  )
 *   if True:	run thread ff_display_spectrum() to display audio spectrum
 *   if False:	disable it.
 */
//static bool enable_audio_spectrum=false;

/* seek position */
//long start_tmsecs=60*55; /*in sec, starting position */

/* param: ( enable_avfilter )
 *   if True:	display_window rotation and size will be adjusted according to avfilter descr.
 *   if False:	use SWS, display_window W&H map to LCD W&H(row&colum), window size will be adjusted to fit for
 *		LCD W&H.
 */
/* enable AVFilter for video */
static bool enable_avfilter=false;//true;

/* param: ( enable_auto_rotate ) ( precondition: enable_avfilter==1 )
 *   if True:	1. auto. map original video long side to LCD_HEIGHT, and short side to LCD_WIDTH.
 *		2. here we assume that LCD_HEIGHT > LCD_WIDTH.
 *		3. if 1, then enable_avfilter also MUST set to 1.
 *
 *   if False:	disable it.
 */
static bool enable_auto_rotate=false;

/*  param: ( transpose_clock ) :  ( precondition: enable_avfilter=1, enable_auto_rotate=false)
 *  if 0, transpose not applied,
	  !!!  NOTE: if enable_auto_rotate=true, then it will be decided by checking image H and W,
	  if image H>W, transpose_colck=0, otherwise transpose_clock=1.
 *  if 1, enable_avfilter MUST be 1, and transpose clock or cclock.
 */
static unsigned char transpose_clock=3; /* when 0, make sure enable_auto_rotate=false !!!
			       * 1 --- rotate clockwise 90deg.
			       * 2 --- rotate clockwise 180deg.
			       * 3 --- rotate clockwise 270deg.
			       */

/* param: ( enable_stretch )
 *   if True:	stretch the image to fit for expected H&W, original image ratio is ignored.
 *   if False:	keep original ratio.
 */
static bool enable_stretch=false;

/* param: ( enable_seek_loop )
 *   True:	loop one single file/stream forever.
 *   False:	play one time only.
 *   NOTE: 1. if TRUE, then curretn input seek postin will be ignored.!!! It always start from the
 *     	      very beginning of the file.
 *	   2. if enable_shuffle set to be true, then enable_seekloop will be false, and viceversa.
 */
/* loop seeking and playing from the start of the same file */
static bool enable_seekloop=false;

/* param: ( enable_shuffle )
 *   True:	play files in the list randomly.
 *   False:	play files in order.
 *   Note:	if enable_shuffle set to be true, then enable_seekloop will be false, and viceversa.
 */
static bool enable_shuffle=false;

/* param: ( enable_filesloop )
 *   True:      Loop playing file(s) in playlist forever, if the file is unrecognizable then skip it
 *		and continue to try next one.
 *   False:	play one time for every file in playlist.
 */
static bool enable_filesloop=true;

/* param:
 *   if True:	disbale audio/video playback.
 *   if False:	enable audio/video playback.
 */
static bool disable_audio=false;
static bool disable_video=false;


/* param: ( disable_fltp )
 *   if True:	disbale AV_SAMPLE_FMT_FLTP playback.
 *   if False:	enable AV_SAMPLE_FMT_FLTP playback..
 */
static bool disable_fltp=false;
/* Force audio to be MONO */
static bool enable_audio_mono=true;

/* param: ( enable_clip_test )
 * NOTE:
 *   Clip test only for media file with audioSrteams, if no audioStream or audioStream is disabled,
 *   then is will only display the first picture from the videoStream and hold on for CLIP_PLAYTIME,
 *   then skip to next file.
 *   if True:	play the beginning of a file for CLIP_PLAYTIME seconds, then skip.
 *   if False:	disable clip test.
 */
static bool enable_clip_test=false;


/* param: ( play mode )
 *   mode_loop_all:	loop all files in the list
 *   mode_repeat_one:	repeat current file
 *   mode_shuffle:	pick next file randomly
 */
static enum ffmotion_mode playmode=mode_loop_all;

/*-----------------------------------------------------
FFplay for most types of media files:
	.mp3, .mp4, .avi, .jpg, .png, .gif, ...
-----------------------------------------------------*/
void * thread_ffplay_motion(EGI_PAGE *page)
//void * thread_ffplay_motion(FFPLAY_CONTEXT *FFmotion_Ctx)
{
        /*  Check ffplay context  */
        if(FFmotion_Ctx==NULL) {
                EGI_PLOG(LOGLV_ERROR,"%s: Context struct FFmotion_Ctx is NULL!", __func__);
                return (void *)-1;
        }
        else if ( FFmotion_Ctx->fpath==NULL || FFmotion_Ctx->fpath[0]==NULL )
        {
                EGI_PLOG(LOGLV_ERROR,"%s: Context struct FFmotion_Ctx has a NULL fpath!", __func__);
                return (void *)-1;
        }
        else if (FFmotion_Ctx->ftotal<=0 )
        {
                EGI_PLOG(LOGLV_ERROR,"%s: No media file in FFmotion_Ctx!", __func__);
                return (void *)-1;
        }


	int ftotal;		/* number of multimedia files */
	int fnum;		/* Index number of files in array FFmotion_Ctx->fpath */
	int fnum_playing;	/* Current playing fpath index, fnum may be changed by command PRE/NEXT */

	const char **fpath=NULL; //FFmotion_Ctx->fpath;  /* array of media file path */
	char *fname=NULL;
	char *fbsname=NULL;

	int ff_sec_Vduration=0; /* in seconds, multimedia file Video duration */
	int ff_sec_Aduration=0; /* in seconds, multimedia file Audio duration */
	//Global int ff_sec_Velapsed=0;  /* in seconds, playing time elapsed for Video, for subtitle synch. */
	int ff_sec_Aelapsed=0;  /* in seconds, playing time elapsed for Audio */

	/* for VIDEO and AUDIO  ::  Initializing these to NULL prevents segfaults! */
	AVFormatContext		*pFormatCtx=NULL;
	AVDictionaryEntry 	*tag=NULL;

	/* for VIDEO  */
	int			i;
	int			videoStream=-1; /* >=0, if stream exists */
	AVCodecContext		*pCodecCtxOrig=NULL;
	AVCodecContext		*pCodecCtx=NULL;
	AVCodec			*pCodec=NULL;
	AVFrame			*pFrame=NULL;
	AVFrame			*pFrameRGB=NULL;
	AVPacket		packet;
	int			frameFinished;
	int			numBytes;
	uint8_t			*buffer=NULL;
	struct SwsContext	*sws_ctx=NULL;
	AVRational 		time_base; /*get from video stream, pFormatCtx->streams[videoStream]->time_base*/

	int Hb,Vb;  /* Horizontal and Veritcal size of a picture */
	/* for Pic Info. */
	struct PicInfo pic_info;
	pic_info.app_page=page; /* for PAGE wallpaper */

	/* Absolute screen size, aligned with original LCD coord X/Y ->W/H */
	int scrnX;
	int scrnY;
	/* Available displaying area in the screen */
	int scrnUseX;
	int scrnUseY;

	/* origin movie/image size, aligned with original LCD coord W/H. */
	int widthOrig;
	int heightOrig;

	/* for scaled movie size, aligned with original LCD coord W/H.
         * Intermediate variables.
         */
	int scwidth;
	int scheight;

	/* display window, the final window_size that will be applied to AVFilter or SWS.
	   0. Display width/height are NOT image upright width/height, their are related to LCD coordinate!
	      If auto_rotation enabled, final display WxH are mapped to LCD row_pixel_number x column_pixel_number .
	   1. Display_width/height are limited by scwidth/scheight, which are decided by original movie size
	      and LCD size limits.
	   2. When disable AVFILTER, display_width/disaply_height are just same as scwidth/scheight.
        */
//	int display_width;
//	int display_height;

	/* width and height for SWS output */
	int sws_width;
	int sws_height;


	/*  for AUDIO  ::  for audio   */
	int			audioStream=-1;/* >=0, if stream exists */
	AVCodecContext		*aCodecCtxOrig=NULL;
	AVCodecContext		*aCodecCtx=NULL;
	AVCodec			*aCodec=NULL;
	AVFrame			*pAudioFrame=NULL;

	/* for AVCodecDescriptor */
	enum AVCodecID		 vcodecID,acodecID;
	const AVCodecDescriptor	 *vcodecDespt=NULL;
	const AVCodecDescriptor	 *acodecDespt=NULL;

	int			frame_size;
	int 			sample_rate;
	int			out_sample_rate; /* after conversion, for ffplaypcm */
	enum AVSampleFormat	sample_fmt;
	int			nb_channels;
	int			bytes_per_sample;
	int64_t			channel_layout;
	int			nchanstr=256;
	char			chanlayout_string[256];
	int 			bytes_used;
	int			got_frame;
	struct SwrContext		*swr=NULL; /* AV_SAMPLE_FMT_FLTP format conversion */
	uint8_t			*outputBuffer=NULL;/* for converted data */
	int 			outsamples;

	/* for AVFilters */
	AVFilterContext *avFltCtx_BufferSink=NULL;
	AVFilterContext *avFltCtx_BufferSrc=NULL;
	AVFilter	*avFlt_BufferSink=NULL; /* to be freed by its holding graph */
	AVFilter	*avFlt_BufferSrc=NULL;
	AVFilterInOut	*avFltIO_InPuts=NULL;/* A linked-list of the inputs/outputs of the filter chain */
	AVFilterInOut	*avFltIO_OutPuts=NULL;
	AVFilterGraph	*filter_graph=NULL; /* filter chain graph */
	AVFrame		*filt_pFrame=NULL; /* for filtered frame */
	enum AVPixelFormat outputs_pix_fmts[] = { AV_PIX_FMT_RGB565LE, AV_PIX_FMT_NONE };/* NONE as for end token */
	char args[512];

	/* video filter descr, same as -vf option in ffmpeg */
	/* scale = Width x Height */
	/* transpose: transpose rows with columns in the input video */


	/* for AVFilter description, which will be parsed to apply on movie frames */
	char filters_descr[512]={0};

	/* time structe */
	struct timeval tm_start, tm_end;

	/* thread for displaying RGB data */
	pthread_t pthd_displayPic;
	pthread_t pthd_displaySub;
//	pthread_t pthd_audioSpectrum;
	bool pthd_displayPic_running=false;
	bool pthd_subtitle_running=false;
//	bool pthd_audioSpectrum_running=false;

	char *pfsub=NULL; /* subtitle path */
	int ret;

	EGI_PDEBUG(DBG_FFPLAY," start ffplay with input ftotal=%d.\n", ftotal);

	/* check expected display window size */
	if(enable_avfilter) {
		if( (display_width&0xF) != 0 || (display_height&0xF) !=0 ) {
			EGI_PLOG(LOGLV_WARN,"ffplay: WARING!!! Size of display_window side must be multiples of 16 for AVFiler.\n");
		}
	}
	else if( (display_width&0x1) != 0 || (display_height&0x1) !=0 ) {
			EGI_PLOG(LOGLV_WARN,"ffplay: WARING!!! Size of display_window side must be multiples of 2 for SWS.\n");
	}


        /* prepare fb device just for FFPLAY */
        init_fbdev(&ff_fb_dev);
	ff_fb_dev.pixcolor_on=true; 	  /* Use private pixcolor */

	/* Get screen size, absolute position/size */
	scrnX=ff_fb_dev.vinfo.xres; /* Use system vinfo */
	scrnY=ff_fb_dev.vinfo.yres;

	/* Get available/useful area for displaying */
	scrnUseX=scrnX-scrnMargX;
	scrnUseY=scrnY-scrnMargY;

	/* If SWS is disabled, then set display_width and display_height to MAX. size  */
	if(disable_scale_size) {
		display_width=scrnUseX;
		display_height=scrnUseY;
	}

	/* Fit display_width * display_height into (scrnX-scrnMargX)*(scrnY-scrnMargY) */
//	if(display_height>scrnY-scrnMargY)
//		display_height=scrnY-scrnMargY;
//	if(display_width>scrnX-scrnMargX)
//		display_width=scrnX-scrnMargX;

        if( display_width > scrnUseX || display_height > scrnUseY ) {
		if( (1.0*display_width/display_height) >= (1.0*scrnUseX/scrnUseY) )
		{
			/* fit for width, only if display_width > scrnX-scrnMargX, keep ratio */
			if(display_width>scrnUseX) {
				scwidth=scrnUseX;
				scheight=scwidth*display_height/display_width;
			}
		}
		else if ( (1.0*display_height/display_width) > (1.0*scrnUseY/scrnUseX) )
		{
			/* fit for height, only if video height > screen height, keep ratio */
			if(display_height>scrnUseY) {
				scheight=scrnUseY;
				scwidth=scheight*display_width/display_height;
			}
		}

		/* re_assign back */
		display_width=scwidth;
		display_height=scheight;
	}
	/* ELSE:  keep original display_width*display_height size */

	/* roate displaying area and PAGE */
	fb_position_rotate(&ff_fb_dev, transpose_clock);  /* rotate displaying position */
	motpage_rotate(transpose_clock);  /* rotate main PAGE */

/*<<<<<<<<<<<<<<<<<<<<<<<< 	 LOOP PLAYING LIST    >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/
/* loop playing all files, check if enable_filesloop==true at the end of while(1) */
while(1) {

        /* For Each Loop:  Check ffplay context and re_gain fpath */
        if(FFmotion_Ctx==NULL) {
                EGI_PLOG(LOGLV_ERROR,"%s: Context struct FFmotion_Ctx is NULL!", __func__);
                return (void *)-1;
        }
        else if ( FFmotion_Ctx->fpath==NULL || FFmotion_Ctx->fpath[0]==NULL )
        {
                EGI_PLOG(LOGLV_ERROR,"%s: Context struct FFmotion_Ctx has a NULL fpath!", __func__);
                return (void *)-1;
        }
        else if (FFmotion_Ctx->ftotal<=0 )
        {
                EGI_PLOG(LOGLV_ERROR,"%s: No media file in FFmotion_Ctx!", __func__);
                return (void *)-1;
        }

	/* Get URL/fpath from FFmotion context */
	if( FFmotion_Ctx->utotal > 0 ) {	/* URL first */
		fpath=FFmotion_Ctx->url;
		ftotal=FFmotion_Ctx->utotal;
	} else {
		fpath=FFmotion_Ctx->fpath;  	/* Local files then */
		ftotal=FFmotion_Ctx->ftotal;
	}

   	/* Register all formats and codecs, before loop for() is OK!! ??? */
	EGI_PLOG(LOGLV_INFO,"%s: Init and register codecs ... \n",__func__);
   	av_register_all();
   	avformat_network_init();
	if(enable_avfilter) {
		avfilter_register_all(); /* register all default builtin filters */
   	}

   /* play all input files, one by one. */
   for(fnum=0; fnum < ftotal; fnum++)
   {
	/* check if enable_shuffle */
	if(enable_shuffle) {
		fnum=egi_random_max(ftotal)-1;
	}

	/* clear displaying area */
//	fbset_color(WEGI_COLOR_BLACK);
//	draw_filled_rect(&ff_fb_dev, 0,30, 239,265);

	/* refresh page */
	egi_page_needrefresh(page);


	/* reset elaped time recorder */
	ff_sec_Velapsed=0;

	vcodecID=AV_CODEC_ID_NONE;

	/* Open media stream or file */
	EGI_PLOG(LOGLV_INFO,"%s: [fnum=%d] Start to open file %s...\n", __func__, fnum, fpath[fnum]);
	if(avformat_open_input(&pFormatCtx, fpath[fnum], NULL, NULL)!=0)
	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to open the file, or file type is not recognizable.\n",__func__);

	   if(enable_filesloop) {	/* if loop, skip to next one */
		avformat_close_input(&pFormatCtx);
        	pFormatCtx=NULL;
		tm_delayms(10); /* sleep less time if try to catch a key_frame for a media stream */
		continue;
	   }
	   else {
		return (void *)-1;
	   }
	}

	/* Retrieve stream information, !!!!!  time consuming  !!!! */
	EGI_PDEBUG(DBG_FFPLAY,"%lld(ms):  Retrieving stream information... \n", tm_get_tmstampms());
	/* ---- seems no use! ---- */
	//pFormatCtx->probesize2=128*1024;
	//pFormatCtx->max_analyze_duration2=8*AV_TIME_BASE;
	if(avformat_find_stream_info(pFormatCtx, NULL)<0) {
		EGI_PLOG(LOGLV_ERROR,"Fail to find stream information!\n");
#ifdef ENABLE_MEDIA_LOOP
		avformat_close_input(&pFormatCtx);
        	pFormatCtx=NULL;
		continue;
#else
		return (void *)-1;
#endif
	}


	/* Dump information about file onto standard error */
	EGI_PDEBUG(DBG_FFPLAY,"%lld(ms): Try to dump file information... \n",tm_get_tmstampms());
	av_dump_format(pFormatCtx, 0, fpath[fnum], 0);

	/* OR to read dictionary entries one by one */
#if 0
	while( tag=av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX) ) {
		EGI_PDEBUG(DBG_FFPLAY,"metadata: key [%s], value [%s] \n", tag->key, tag->value);
	}
#endif


	/* Find the first video stream and audio stream */
	EGI_PDEBUG(DBG_FFPLAY,"%lld(ms):	Try to find the first video stream... \n",tm_get_tmstampms());
	/* reset stream index first */
	videoStream=-1;
	audioStream=-1;

	//printf("%d streams found.\n",pFormatCtx->nb_streams);
	/* find the first available stream of VIDEO and AUDIO */
	for(i=0; i<pFormatCtx->nb_streams; i++) {
		/* For VIDEO streams */
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO ) {
		   if( videoStream < 0) {
			videoStream=i;
			EGI_PDEBUG(DBG_FFPLAY,"Video is found in stream[%d], to be accepted.\n",i);
			vcodecID=pFormatCtx->streams[i]->codec->codec_id;
			/*----------- Picture ----------
			 	AV_CODEC_ID_MJPEG
			 	AV_CODEC_ID_MJPEGB
			 	AV_CODEC_ID_LJPEG
				AV_CODEC_ID_JPEGLS
				AV_CODEC_ID_BMP
				AV_CODEC_ID_PNG
				... ...
			-------------------------------*/
		        vcodecDespt=avcodec_descriptor_get(vcodecID);
			if(vcodecDespt != NULL) {
				EGI_PLOG(LOGLV_INFO,"%s: Video codec name: %s, %s\n",
							__func__, vcodecDespt->name, vcodecDespt->long_name);
			}
		   }
		   else
			EGI_PDEBUG(DBG_FFPLAY,"Video is also found in stream[%d], to be ignored.\n",i);
		}
		/* For AUDIO streams */
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO ) {
		   if( audioStream < 0) {
			audioStream=i;
			EGI_PDEBUG(DBG_FFPLAY,"Audio is found in stream[%d], to be accepted.\n",i);
			acodecID=pFormatCtx->streams[i]->codec->codec_id;
			/*----------- AUDIO  ----------
				AV_CODEC_ID_MP2
				AV_CODEC_ID_MP3
				AV_CODEC_ID_AAC
				AV_CODEC_ID_AC3
				AV_CODEC_ID_FLAC
				AV_CODEC_ID_MP3ADU
				AV_CODEC_ID_MP3ON4
				AV_CODEC_ID_SPEEX
				... ...
			-------------------------------*/
		        acodecDespt=avcodec_descriptor_get(acodecID);
			if(acodecDespt != NULL) {
				EGI_PLOG(LOGLV_INFO,"%s: Audio codec name: %s, %s\n",
							__func__, acodecDespt->name, acodecDespt->long_name);
			}
		   }
		   else
			EGI_PDEBUG(DBG_FFPLAY,"Audio is also found in stream[%d], to be ignored.\n",i);
		}
	}
	EGI_PLOG(LOGLV_INFO,"%s: get videoStream [%d], audioStream [%d] \n", __func__,
				 videoStream, audioStream);

	if(videoStream == -1) {
		EGI_PLOG(LOGLV_INFO,"Didn't find a video stream!\n");
//		return (void *)-1;
	}
	if(audioStream == -1) {
		EGI_PLOG(LOGLV_WARN,"Didn't find an audio stream!\n");
//go on   	return (void *)-1;
	}

	if(videoStream == -1 && audioStream == -1) {
		EGI_PLOG(LOGLV_ERROR,"No stream found for video or audio! quit ffplay now...\n");
		return (void *)-1;
	}

	/* Display file(MP3) name, if videoStream is available then it will be cleared later! */
	//if( audioStream>=0 && disable_video )
	fname=strdup(fpath[fnum]);
	printf("fname:%s\n",fname);
	fbsname=basename(fname);
        FTsymbol_uft8strings_writeFB(&ff_fb_dev, egi_appfonts.regular,  /* FBdev, fontface */
                                    18, 18, fbsname,               	/* fw,fh, pstr */
                                    ff_fb_dev.pos_xres, 1, 0,  		/* pixpl, lines, gap */
                                    5, 5,                      		/* x0,y0, */
                                    WEGI_COLOR_GRAYB, -1, -1);   	/* fontcolor, stranscolor,opaque */
	pic_info.fname=fbsname;
	//free(fname); fname=NULL; /* to be freed at last */

/* disable audio */
if(disable_audio && audioStream>=0 )
{
	EGI_PDEBUG(DBG_FFPLAY,"Audio is disabled by the user! \n");
	audioStream=-1;
}

	/* proceed --- audio --- stream */
	if(audioStream >= 0) /* only if audioStream exists */
  	{
		EGI_PLOG(LOGLV_INFO,"%s: Prepare for audio stream processing ...\n",__func__);
		/* Get a pointer to the codec context for the audio stream */
		aCodecCtxOrig=pFormatCtx->streams[audioStream]->codec;
		/* Find the decoder for the audio stream */
		EGI_PLOG(LOGLV_INFO,"%s: Try to find the decoder for the audio stream... \n",__func__);
		aCodec=avcodec_find_decoder(aCodecCtxOrig->codec_id);
		if(aCodec == NULL) {
			EGI_PLOG(LOGLV_ERROR, "Unsupported audio codec! quit ffplay now...\n");
			return (void *)-1;
		}
		/* copy audio codec context */
		aCodecCtx=avcodec_alloc_context3(aCodec);
		if(avcodec_copy_context(aCodecCtx, aCodecCtxOrig) != 0) {
			EGI_PLOG(LOGLV_ERROR, "Couldn't copy audio code context! quit ffplay now...\n");
			return (void *)-1;
		}
		/* open audio codec */
		EGI_PLOG(LOGLV_INFO,"%s: Try to open audio stream with avcodec_open2()... \n",__func__);
		if(avcodec_open2(aCodecCtx, aCodec, NULL) <0 ) {
			EGI_PLOG(LOGLV_ERROR, "Could not open audio codec with avcodec_open2(), quit ffplay now...!\n");
			return (void *)-1;
		}
		/* get audio stream parameters */
		frame_size = aCodecCtx->frame_size;
		sample_rate = aCodecCtx->sample_rate;
		sample_fmt  = aCodecCtx->sample_fmt;
		bytes_per_sample = av_get_bytes_per_sample(sample_fmt);
		nb_channels = aCodecCtx->channels;
		channel_layout = aCodecCtx->channel_layout;
		av_get_channel_layout_string(chanlayout_string, nchanstr, nb_channels, channel_layout);
		EGI_PDEBUG(DBG_FFPLAY,"		frame_size=%d\n",frame_size);//=nb_samples, nb of samples per frame.
		EGI_PDEBUG(DBG_FFPLAY,"		channel_layout=%lld, as for: %s\n",
								 channel_layout, chanlayout_string);
		EGI_PDEBUG(DBG_FFPLAY,"			   %s\n", chanlayout_string);
		EGI_PDEBUG(DBG_FFPLAY,"		nb_channels=%d\n",nb_channels);
		EGI_PDEBUG(DBG_FFPLAY,"		sample format: %s\n",av_get_sample_fmt_name(sample_fmt) );
		EGI_PDEBUG(DBG_FFPLAY,"		bytes_per_sample: %d\n",bytes_per_sample);
		EGI_PDEBUG(DBG_FFPLAY,"		sample_rate=%d\n",sample_rate);

		if( nb_channels > 2 ) {
			EGI_PDEBUG(DBG_FFPLAY," !!! Number of audio channels is %d >2, not supported!\n",nb_channels);
			goto FAIL_OR_TERM;
		}

		/* Disable audio if audio format is FLTP */
		if( sample_fmt == AV_SAMPLE_FMT_FLTP && disable_fltp ) {
			EGI_PLOG(LOGLV_WARN,"%s: Disable audio for AV_SAMPLE_FMT_FLTP!",__func__);
			audioStream=-1;
		}
		/* prepare SWR context for FLTP format conversion, WARN: float points operations!!!*/
		else if(sample_fmt == AV_SAMPLE_FMT_FLTP ) {

			/* Just same */
			out_sample_rate=sample_rate;

#if 1 /* -----METHOD (1)-----:  or to be replaced by swr_alloc_set_opts() */
			EGI_PLOG(LOGLV_INFO,"%s: alloc swr and set_opts for converting AV_SAMPLE_FMT_FLTP to S16 ...\n",
									__func__);
			swr=swr_alloc();
			av_opt_set_channel_layout(swr, "in_channel_layout",  channel_layout, 0);
			/* out channel layout to be mono */
			if(enable_audio_mono)
				av_opt_set_channel_layout(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
			else
				av_opt_set_channel_layout(swr, "out_channel_layout", channel_layout, 0);
			av_opt_set_int(swr, "in_sample_rate", 	sample_rate, 0); // for FLTP sample_rate = 24000
			av_opt_set_int(swr, "out_sample_rate", 	out_sample_rate, 0);
			av_opt_set_sample_fmt(swr, "in_sample_fmt",   AV_SAMPLE_FMT_FLTP, 0);
			av_opt_set_sample_fmt(swr, "out_sample_fmt",  AV_SAMPLE_FMT_S16P, 0);

#else  /* -----METHOD (2)-----:  call swr_alloc_set_opts() */
	/* Function definition:
	struct SwrContext *swr_alloc_set_opts( swr ,
                           int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
                           int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
                           int log_offset, void *log_ctx);		 */

			/* allocate and set opts for swr */
			EGI_PLOG(LOGLV_INFO,"%s: swr_alloc_set_opts()...\n",__func__);
			swr=swr_alloc_set_opts( swr,
						channel_layout,AV_SAMPLE_FMT_S16P,  out_sample_rate,
						channel_layout,AV_SAMPLE_FMT_FLTP,  sample_rate,
						0, NULL );

			/* how to dither ...?? */
			//av_opt_set(swr,"dither_method",SWR_DITHER_RECTANGULAR,0);
#endif

			EGI_PLOG(LOGLV_INFO,"%s: start swr_init() ...\n", __func__);
			swr_init(swr);

			/* alloc outputBuffer */
			EGI_PDEBUG(DBG_FFPLAY,"%s: malloc outputBuffer ...\n",__func__);
			outputBuffer=malloc(2*frame_size * bytes_per_sample);
			if(outputBuffer == NULL)
	       	 	{
				EGI_PLOG(LOGLV_ERROR,"%s: malloc() outputBuffer failed!\n",__func__);
				return (void *)-1;
			}

			/* open pcm play device and set parameters */
 			if( egi_prepare_pcm_device(nb_channels,out_sample_rate,false) !=0 ) /* true for interleaved access */
			{
				EGI_PLOG(LOGLV_ERROR,"%s: fail to prepare pcm device for interleaved access.\n",
											__func__);
				goto FAIL_OR_TERM;
			}
		}
		else
		{
			/* Directly open pcm play device and set parameters */
 			if ( egi_prepare_pcm_device(nb_channels,sample_rate,false) !=0 ) /* 'false' as for 'noninterleaved access' */
			{
				EGI_PLOG(LOGLV_ERROR,"%s: fail to prepare pcm device for noninterleaved access.\n",
											__func__);
				goto FAIL_OR_TERM;
			}
		}

		/* Before allocate frame for audio, re_check audioStream as it may be reset! */
		if(audioStream >=0 ) {
			EGI_PDEBUG(DBG_FFPLAY,"%s: av_frame_alloc() for Audio...\n",__func__);
			pAudioFrame=av_frame_alloc();
			if(pAudioFrame==NULL) {
				EGI_PLOG(LOGLV_ERROR, "Fail to allocate pAudioFrame!\n");
				return (void *)-1;
			}
		}

		/* <<<<<<<<<<<<     create a thread to display audio spectrum (pending)   >>>>>>>>>>>>>>> */



	} /* end of if(audioStream >0 ) */


/* disable video */
if(disable_video && videoStream>=0 )
{
	EGI_PDEBUG(DBG_FFPLAY,"Video is disabled by the user! \n");
	videoStream=-1;
}

     /* proceed --- video --- stream */
    if(videoStream >=0 ) /* only if videoStream exists */
    {
	EGI_PDEBUG(DBG_FFPLAY,"Prepare for video stream processing ...\n");

	/* get time_base */
	time_base = pFormatCtx->streams[videoStream]->time_base;

	/* Get a pointer to the codec context for the video stream */
	pCodecCtxOrig=pFormatCtx->streams[videoStream]->codec;

	/* Find the decoder for the video stream */
	EGI_PDEBUG(DBG_FFPLAY,"Try to find the decoder for the video stream... \n");
	pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if(pCodec == NULL) {
		EGI_PLOG(LOGLV_WARN,"Unsupported video codec! try to continue to decode audio...\n");
		videoStream=-1;
		//return (void *)-1;
	}
	/* if video size is illegal, then reset videoStream to -1, to ignore video processing */
	if(pCodecCtxOrig->width <= 0 || pCodecCtxOrig->height <= 0) {
		EGI_PLOG(LOGLV_WARN,"Video width=%d or height=%d illegal! try to continue to decode audio...\n",
				   pCodecCtxOrig->width, pCodecCtxOrig->height);
		videoStream=-1;
	}

    }

    /* videoStream may be reset to -1, so confirm again */
    if(videoStream >=0 && pCodec != NULL)
    {

	/* copy video codec context */
	pCodecCtx=avcodec_alloc_context3(pCodec);
	if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
		EGI_PLOG(LOGLV_ERROR,"Couldn't copy video code context!\n");
		return (void *)-1;
	}

	/* open video codec */
	if(avcodec_open2(pCodecCtx, pCodec, NULL) <0 ) {
		EGI_PLOG(LOGLV_WARN, "Cound not open video codec!\n");
		return (void *)-1;
	}

	/* Allocate video frame */
	pFrame=av_frame_alloc();
	if(pFrame==NULL) {
		EGI_PLOG(LOGLV_ERROR,"Fail to allocate pFrame!\n");
		return (void *)-1;
	}
	/* allocate an AVFrame structure */
	EGI_PDEBUG(DBG_FFPLAY,"Try to allocate an AVFrame structure for RGB data...\n");
	pFrameRGB=av_frame_alloc();
	if(pFrameRGB==NULL) {
		EGI_PLOG(LOGLV_ERROR, "Fail to allocate a AVFrame struct for RGB data !\n");
		return (void *)-1;
	}
	/* get original video size */
	EGI_PDEBUG(DBG_FFPLAY,"original video image size: width=%d, height=%d\n",
					 		pCodecCtx->width,pCodecCtx->height);



/* if enable_auto_rotate, then map long side to H, and shore side to W
 * here assume LCD_H > LCD_W, and FB linesize is pixel number of LCD_W.
 */
if(enable_auto_rotate)
{
	if( pCodecCtx->height >= pCodecCtx->width ) /*if image upright H>W */
		transpose_clock=0;
	else
		transpose_clock=1;
}

/* get original video size, swap width and height if clock/cclock_transpose
 * pCodecCtx->heidth and width is the upright image size.
 */
if(enable_avfilter)
{
	if(transpose_clock) { /* if clock/cclock, swap H & W */
		widthOrig=pCodecCtx->height;
		heightOrig=pCodecCtx->width;
	}
	else {
		widthOrig=pCodecCtx->width;
		heightOrig=pCodecCtx->height;
	}
}
else
{
	/* Landscape modem, align image height to screen width */
	if(transpose_clock & 0x1) {
		widthOrig=pCodecCtx->height;
		heightOrig=pCodecCtx->width;
	}
	/* Portrait mode */
	else {
		widthOrig=pCodecCtx->width;
		heightOrig=pCodecCtx->height;
	}
}
EGI_PDEBUG(DBG_FFPLAY,"Rotated video size: widthOrig=%d, heightOrig=%d \n",widthOrig, heightOrig);

/* Check original size and re_set disable_scale_size to FALSE */
if( widthOrig > scrnUseX || heightOrig > scrnUseY )
	disable_scale_size=false;


/* Calculate scwidth and scheight */
if(enable_stretch)  /* stretch image size to expected */
{
	/* <<<<<<< stretch image size, will NOT keep H/W ratio !!!  >>>>>> */
	scwidth=widthOrig;
	scheight=heightOrig;
        if( widthOrig > scrnX-scrnMargX ) {
		scwidth=scrnX-scrnMargX;
	}
	if( heightOrig > scrnY-scrnMargY) {
		scheight=scrnY-scrnMargY;
	}
}
else /* if NOT stretch, then keep original ratio,and fit into display_width*display_height */
{
	/* <<<<<<<   calculate scaled movie size to fit into display_height*display_width >>>>>>>
	 * 1. widthOrig and heightOrig are the original image size,
	 *    widthOrig and heightOrig are aligned with screen H and W respectively.
	 * 2. scwidth and scheight are scaled/converted to be the best fit size into
	 *     display_width*display_height, which are already well fitted into scrnUseX*scrnUseY.
         */
	if( widthOrig > display_width || heightOrig > display_height )
	{
		if( (1.0*widthOrig/heightOrig) >= (1.0*display_width/display_height) )
		{
			/* Fit for width, only if widthOrig > display_width */
			if( widthOrig > display_width ) {
				scwidth=display_width;
				scheight=heightOrig*scwidth/widthOrig;
			}
		}
		else if( (1.0*heightOrig/widthOrig) >= (1.0*display_height/display_width) )
		{
			/* Fit for height, only if heightOrig > display_height */
			if( heightOrig > display_height ) {
				scheight=display_height;
				scwidth=widthOrig*scheight/heightOrig;
			}
		}

	}
	else {
		/* keep original movie/image size */
	 	scwidth=widthOrig;
	 	scheight=heightOrig;
	}
}
EGI_PDEBUG(DBG_FFPLAY,"Fit and scale video size into display_widthxdisplay_height: scwidth=%d, scheight=%d \n",
											scwidth, scheight);

	/* <<<<<------   Get final displaying area size   ----->>>>> */
	if( disable_scale_size ) {
		/* re_assign back to display_width and display_height */
		display_width=widthOrig;
		display_height=heightOrig;
	}
	else {
		/* re_assign back to display_width and display_height */
		display_width=scwidth;
		display_height=scheight;
	}



/*  Double check here, should alread have been checked at the very begin
 *  reset display window size to be multiples of 4(for AVFilter Descr) or 2(for SWS).
*/
if(enable_avfilter)
{
   	display_width=(display_width>>2)<<2;  /*4?*/
   	display_height=(display_height>>2)<<2;
}
else
{
	display_width=(display_width>>2)<<2;
	display_height=(display_height>>2)<<2;
}

EGI_PDEBUG(DBG_FFPLAY,"disable_scale_size: %s;  Final display area size: W%d*H%d \n",
		 				(disable_scale_size)?"YES":"NO", display_width, display_height);


	/* Addjust displaying window position */
	/* Landscape mode */
	if(transpose_clock & 0x1 ) {
		offx=((ff_fb_dev.vinfo.yres-scrnMargY-display_height)>>1)
					+ ( (transpose_clock==1)?scrnMargY_Upp:scrnMargY_Down );
		offy=((ff_fb_dev.vinfo.xres-scrnMargX-display_width)>>1)
					+ ( (transpose_clock==1)?scrnMargX_Right:scrnMargX_Left );
		EGI_PDEBUG(DBG_FFPLAY," yres=%d, xres=%d, offx=%d,  offy=%d  as of current coord.\n",
						ff_fb_dev.vinfo.yres, ff_fb_dev.vinfo.xres, offx, offy);
	}
	/* Portrait mode */
        else {
		offx=(ff_fb_dev.vinfo.xres-display_width)>>1;
		if(IS_IMAGE_CODEC(vcodecID)) 		/* for IMAGE */
			offy=((265-29-display_height)>>1) +30;
		else 					/* for MOTION PIC */
			offy=50;
	}


	/* Determine required buffer size and allocate buffer for scaled picture size */
	numBytes=avpicture_get_size(PIX_FMT_RGB565LE, display_width, display_height);//pCodecCtx->width, pCodecCtx->height);
	pic_info.numBytes=numBytes;
	/* malloc buffer for pFrameRGB */
	buffer=(uint8_t *)av_malloc(numBytes);

	/* <<<<<<<<    allocate mem. for PIC buffers   >>>>>>>> */
	if(malloc_PicBuffs(display_width,display_height,2) == NULL) { /* pixel_size=2bytes for PIX_FMT_RGB565LE */
		EGI_PLOG(LOGLV_ERROR,"Fail to allocate memory for PICbuffs!\n");
		return (void *)-1;
	}
	else
		EGI_PDEBUG(DBG_FFPLAY, "finish allocate memory for uint8_t *PICbuffs[%d]\n",PIC_BUFF_NUM);



	/*<<<<<<<<<<<<<     Hs He Vs Ve for IMAGE layout on LCD    >>>>>>>>>>>>>>>>*/
	 Hb=offx;
	 Vb=offy;
	 pic_info.Hs=Hb;
	 pic_info.Vs=Vb;

	/* NOte: V, H is aligned with image coord. */
	 if(transpose_clock & 0x1) {		/* Landscape mode */
		pic_info.Ve=Vb+display_width-1;
		pic_info.He=Hb+display_height-1;
	 }
	 else {					/* Portrait mode */
	 	 pic_info.Ve=Vb+display_height-1;
		 pic_info.He=Hb+display_width-1;
	 }

	 pic_info.vcodecID=vcodecID;

	 /* Assign appropriate parts of buffer to image planes in pFrameRGB
	 Note that pFrameRGB is an AVFrame, but AVFrame is a superset of AVPicture */
	 if(transpose_clock)
		 avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB565LE, display_height, display_width); //pCodecCtx->width, pCodecCtx->height);
	 else
		 avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB565LE, display_width, display_height); //pCodecCtx->width, pCodecCtx->height);

if(!enable_avfilter) /* use SWS, if not AVFilter */
{
	/* Initialize SWS context for software scaling, allocate and return a SwsContext */
	EGI_PDEBUG(DBG_FFPLAY, "Initialize SWS context for software scaling... \n");

	if(transpose_clock & 0x1) {	/* Landscap mode */
		sws_width=display_height;
		sws_height=display_width;
	} else {			/* Portrait mode */
		sws_width=display_width;
		sws_height=display_height;
	}
        printf("---- SWS_CTX: sws_width=%d, sws_height=%d ---- \n", sws_width, sws_height);

	sws_ctx = sws_getContext( pCodecCtx->width,
				  pCodecCtx->height,
				  pCodecCtx->pix_fmt,
				  sws_width, 		/* scaled output size */
				  sws_height,
				  AV_PIX_FMT_RGB565LE,
				  SWS_BILINEAR,
				  NULL,
				  NULL,
				  NULL
				);

//	av_opt_set(sws_ctx,"dither_method",SWR_DITHER_RECTANGULAR,0);
}

/* <<<<<<<<<<<<     create a thread to display picture to LCD    >>>>>>>>>>>>>>> */
/* Even if no video stream */
	if(pthread_create(&pthd_displayPic,NULL,thdf_Display_motionPic,(void *)&pic_info) != 0) {
		EGI_PLOG(LOGLV_ERROR, "Fails to create thread for displaying pictures! \n");
		return (void *)-1;
	}
	else
		EGI_PDEBUG(DBG_FFPLAY,"Finish creating thread for displaying pictures.\n");
	/* set running token */
	pthd_displayPic_running=true;

/* <<<<<<<<<<<<     create a thread to display subtitles     >>>>>>>>>>>>>>> */
	/* check if substitle file *.srt exists in the same path of the media file */
	if(pfsub!=NULL){
		free(pfsub);
		pfsub=NULL;
	}
	pfsub=cstr_dup_repextname(fpath[fnum],".srt");
	EGI_PLOG(LOGLV_CRITICAL,"Subtitle file: %s,  Access: %s\n",pfsub, ( access(pfsub,F_OK)==0 ) ? "OK" : "FAIL" );
	if( pfsub != NULL && access(pfsub,F_OK)==0 ) {
		if( pthread_create(&pthd_displaySub,NULL,thdf_Display_Subtitle,(void *)pfsub ) != 0) {
			EGI_PLOG(LOGLV_ERROR, "Fails to create thread for displaying subtitles! \n");
			pthd_subtitle_running=false;
			free(pfsub); pfsub=NULL;
			//Go on anyway. //return (void *)-1;
		}
		else {
			EGI_PDEBUG(DBG_FFPLAY,"Finish creating thread for displaying subtitles.\n");
			pthd_subtitle_running=true;
		}
	}
	else {
		pthd_subtitle_running=false;
		free(pfsub); pfsub=NULL;
	}


/* if AVFilter ON, then initialize and prepare fitlers */
if(enable_avfilter)
{
  	/*---------<<< START: prepare filters >>>--------*/

   	EGI_PLOG(LOGLV_INFO, "%s: prepare VIDEO avfilters ...\n",__func__);

	/* prepare filters description
	 * WARNING: if original image is much bigger than logo, then the logo maybe invisible after scale.
         */
	if(transpose_clock) /* image WxH to LCD HxW */
	{
		sprintf(filters_descr,"movie=logo.png[logo];[in][logo]overlay=5:5,scale=%d:%d,transpose=clock[out]",
							display_height, display_width);
	}
	else /* image WxH map to LCD WxH */
	{
		sprintf(filters_descr,"movie=logo.png[logo];[in][logo]overlay=5:5,scale=%d:%d[out]",
							display_width, display_height);

	}
	EGI_PLOG(LOGLV_INFO,"AVFilter Descr: %s\n",filters_descr);

	/* initiliaze avfilter graph */
	//printf("start av_frame_alloc() for filtered frame... \n");
	filt_pFrame=av_frame_alloc();/* alloc AVFrame for filtered frame */
	//printf("start avliter_get_by_name()... \n");
   	avFlt_BufferSink=avfilter_get_by_name("buffersink");/* get a registerd builtin filter by name */
   	avFlt_BufferSrc=avfilter_get_by_name("buffer");
   	if(avFlt_BufferSink==NULL || avFlt_BufferSrc==NULL)
   	{
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to get avFlt_BufferSink or avFlt_BufferSrc.\n",__func__);
		goto FAIL_OR_TERM;
   	}
	//printf("start avliter_inout_alloc()... \n");
   	avFltIO_InPuts=avfilter_inout_alloc();
   	avFltIO_OutPuts=avfilter_inout_alloc();
	//printf("start avliter_graph_alloc()... \n");
   	filter_graph=avfilter_graph_alloc();
	if( !avFltIO_InPuts | !avFltIO_OutPuts | !filter_graph )
	{
		EGI_PLOG(LOGLV_ERROR,"fail to alloc filter inputs/outputs or graph.\n");
		goto FAIL_OR_TERM;
	}


   	/* input arguments for filter_graph */
	snprintf(args, sizeof(args),"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			pCodecCtx->width,pCodecCtx->height,pCodecCtx->pix_fmt,
			time_base.num, time_base.den,
			pCodecCtx->sample_aspect_ratio.num, pCodecCtx->sample_aspect_ratio.den );
	EGI_PDEBUG(DBG_FFPLAY,"Set AV Filter graph args as: %s\n",args);

   	/* create source(in) filter in the filter  graph
    	*  int avfilter_graph_create_filter(AVFilterContext **filt_ctx, const AVFilter *filt,
    	*                        const char *name, const char *args, void *opaque,
    	*                        AVFilterGraph *graph_ctx);
   	*/
   	ret=avfilter_graph_create_filter(&avFltCtx_BufferSrc, avFlt_BufferSrc,"in",args,NULL,filter_graph);
   	if(ret<0)
   	{
        	//av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source.\n");
		EGI_PLOG(LOGLV_ERROR,"%s, %s(): Fail to call avfilter_graph_create_filter() to create BufferSrc filter...\n",
							__FILE__, __FUNCTION__ );
		goto FAIL_OR_TERM;
   	}

   	/* create sink(out) filter in the filter chain graph */
   	ret=avfilter_graph_create_filter(&avFltCtx_BufferSink, avFlt_BufferSink,"out",NULL,NULL,filter_graph);
   	if(ret<0)
   	{
        	//av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink.\n");
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to call avfilter_graph_create_filter() to create BufferSink filter...\n",
							__func__);
		goto FAIL_OR_TERM;
   	}

   	/* set output pix format */
   	/**
   	* Set a binary option to an integer list.
   	*
   	* @param obj    AVClass object to set options on
   	* @param name   name of the binary option
   	* @param val    pointer to an integer list (must have the correct type with
   	*               regard to the contents of the list)
  	* @param term   list terminator (usually 0 or -1)
   	* @param flags  search flags
   	*/
   	ret=av_opt_set_int_list(avFltCtx_BufferSink, "pix_fmts", outputs_pix_fmts,
		        	                      AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
   	if (ret < 0) {
        	//av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
		EGI_PLOG(LOGLV_ERROR,"Fail to call av_opt_set_int_list() to set pixel format for output filter...\n");
        	goto FAIL_OR_TERM;
    	}
   	/* set the endpoints for the filter graph --- Source */
   	/* in the view of Caller | in the view of Filter */
   	avFltIO_OutPuts->name		=av_strdup("in");
   	avFltIO_OutPuts->filter_ctx	=avFltCtx_BufferSrc;
   	avFltIO_OutPuts->pad_idx	=0;
   	avFltIO_OutPuts->next		=NULL;

   	/* set the endpoints for the filter graph --- Sink */
   	/* in the view of Caller | in the view of the Filter */
   	avFltIO_InPuts->name	=av_strdup("out");
   	avFltIO_InPuts->filter_ctx	=avFltCtx_BufferSink;
   	avFltIO_InPuts->pad_idx	=0;
   	avFltIO_InPuts->next	=NULL;

   	/* parse filter graph
    	* int avfilter_graph_parse_ptr(AVFilterGraph *graph, const char *filters,
    	*                     AVFilterInOut **open_inputs_ptr, AVFilterInOut **open_outputs_ptr,
    	*                     void *log_ctx)
    	*		                  	( I/O in the view of Caller )
    	*/
    	ret=avfilter_graph_parse_ptr(filter_graph,filters_descr,&avFltIO_InPuts,&avFltIO_OutPuts,NULL);
    	if (ret < 0) {
        	//av_log(NULL, AV_LOG_ERROR, "Fail to parse avfilter graph.\n");
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to call avfilter_graph_parse_ptr() to parse fitler graph descriptions.\n",
							__func__);
        	goto FAIL_OR_TERM;
    	}
    	/* configure the filter graph */
    	ret=avfilter_graph_config(filter_graph,NULL);
    	if (ret < 0) {
        	//av_log(NULL, AV_LOG_ERROR, "Fail to parse avfilter graph.\n");
		EGI_PLOG(LOGLV_ERROR,"Fail to call avfilter_graph_config() to configure filter graph.\n");
        	goto FAIL_OR_TERM;
    	}
    	/* free temp. vars */
    	avfilter_inout_free(&avFltIO_InPuts);
    	avfilter_inout_free(&avFltIO_OutPuts);

  	/*---------<<< END: prepare filters >>>--------*/
	EGI_PDEBUG(DBG_FFPLAY,"Finish prepare avfilter and configure filter graph.\n");

} /* end of AVFilter ON */


  }/* end of (videoStream >=0 && pCodec != NULL) */


	/* Final confirm if any media stream available */
	if(audioStream<0 && videoStream<0)
		goto FAIL_OR_TERM;


        /* Get playing time (duration) */
        if(audioStream>=0) {
                ff_sec_Aduration=atoi( av_ts2timestr(pFormatCtx->streams[audioStream]->duration,
                                                        &pFormatCtx->streams[audioStream]->time_base) );
        }
        if(videoStream>=0) {
                ff_sec_Vduration=atoi( av_ts2timestr(pFormatCtx->streams[videoStream]->duration,
                                                        &pFormatCtx->streams[videoStream]->time_base) );
                EGI_PLOG(LOGLV_TEST,"%s: Video duration is %lds.",__func__,ff_sec_Vduration);
        }


/*  --------  LOOP  ::  Read packets and process data  --------   */

	gettimeofday(&tm_start,NULL);
	EGI_PDEBUG(DBG_FFPLAY,"<<<< ----- FFPLAY START PLAYING STREAMS ----- >>>\n");
	//printf("	 converting video frame to RGB and then send to display...\n");
	//printf("	 sending audio frame data to playback ... \n");
	i=0;

/* if loop playing for only ONE file ..... */
SEEK_LOOP_START:
if(enable_seekloop)
{
	/* seek starting point */
	EGI_PDEBUG(DBG_FFPLAY,"av_seek_frame() to the starting point...\n");
        av_seek_frame(pFormatCtx, 0, 0, AVSEEK_FLAG_ANY);
}
else
{	//pFormatCtx->streams[videoStream]->time_base
	/* Seek to the Video position,
	 * Note:
	 * 1. For MP3, to call av_seek_frame() will fail!
	 */
	if(FFmotion_Ctx->start_tmsecs !=0 ) {
	  av_seek_frame(pFormatCtx, videoStream,(FFmotion_Ctx->start_tmsecs)*time_base.den/time_base.num, AVSEEK_FLAG_ANY);
	}
}


	EGI_PDEBUG(DBG_FFPLAY,"Start while() for loop reading, decoding and playing frames ...\n");
	while( av_read_frame(pFormatCtx, &packet) >= 0) {

		/* -----   process Video Stream   ----- */
		if( videoStream >=0 && packet.stream_index==videoStream)
		{
			//printf("...decoding video frame\n");
			//gettimeofday(&tm_start,NULL);
			if( avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet)<0 )
				EGI_PLOG(LOGLV_ERROR,"Error decoding video, try to carry on...\n");
			//gettimeofday(&tm_end,NULL);
			//printf(" avcode_decode_video2() cost time: %d ms\n",get_costtime(tm_start,tm_end) );

			/* if we get complete video frame(s) */
			if(frameFinished) {

/* If AVFilter ON, then push and pull decoded frames through AVFilter, then send filtered frame RGB data
 *  to pic buff for display.
 */
if(enable_avfilter)
{
				pFrame->pts=av_frame_get_best_effort_timestamp(pFrame);
				/* push decoded frame into the filter graph */
			        if( av_buffersrc_add_frame_flags(avFltCtx_BufferSrc,pFrame,
									AV_BUFFERSRC_FLAG_KEEP_REF) <0 )
				{
				    //av_log(NULL,AV_LOG_ERROR,"Error while feeding pFrame to filter graph through avFltCtx_BufferSrc.\n");
				    EGI_PLOG(LOGLV_ERROR, "Error feeding decoded pFrame to filter graph, try to carry on...\n");
				    //break;
				}

				/* pull filtered frames from the filter graph */
				while(1)
				{
					printf(" av_buffersink_get_frame....\n");
					ret=av_buffersink_get_frame(avFltCtx_BufferSink, filt_pFrame);
					if( ret==AVERROR(EAGAIN) || ret==AVERROR_EOF )
						break;
					else if(ret<0)
					{
				    		EGI_PLOG(LOGLV_WARN, "AVFlilter operation av_buffersink_get_frame()<0, break while()...\n");
						break; /* try to carry on */
						//goto FAIL_OR_TERM;
					}
					/* push data to pic buff for SPI LCD displaying */
					printf(" start Load_Pic2Buff()....\n");
					if( load_Pic2Buff(&pic_info,filt_pFrame->data[0],numBytes) <0 )
						EGI_PDEBUG(DBG_FFPLAY," [%lld] PICBuffs are full! video frame is dropped!\n",
									tm_get_tmstampms());

					av_frame_unref(filt_pFrame); /* unref it, or it will eat up memory */
				}
				av_frame_unref(filt_pFrame);
}
else  /* elif AVFilter OFF, then apply SWS and send scaled RGB data to pic buff for display */
{
				/* convert the image from its native format to RGB */
				//printf("%s: sws_scale converting ...\n",__func__);
				sws_scale( sws_ctx,
					   (uint8_t const * const *)pFrame->data,
					   pFrame->linesize, 0, pCodecCtx->height,
					   pFrameRGB->data, pFrameRGB->linesize
					);

				/* push data to pic buff for SPI LCD displaying */
				//printf("%s: start Load_Pic2Buff()....\n",__func__);
				if( load_Pic2Buff(&pic_info,pFrameRGB->data[0],numBytes) <0 ) {
					EGI_PDEBUG(DBG_FFPLAY,"[%lld] PICBuffs are full! video frame is dropped!\n",
								tm_get_tmstampms());
				}
} /* end of AVFilter ON/OFF */

				/* ---print playing time--- */
				ff_sec_Velapsed=atoi( av_ts2timestr(packet.pts,
				     			&pFormatCtx->streams[videoStream]->time_base) );
				ff_sec_Vduration=atoi( av_ts2timestr(pFormatCtx->streams[videoStream]->duration,
							&pFormatCtx->streams[videoStream]->time_base) );

				//printf("\r	     video Elapsed time: %ds  ---  Duration: %ds  ",
				//					ff_sec_Velapsed, ff_sec_Vduration );
				//printf("\r ------ Time stamp: %llds  ------", packet.pts/ );
				//fflush(stdout);

                                /* --- Reset timing slider ---- */
				motpage_update_timingBar(ff_sec_Velapsed, ff_sec_Vduration);

			} /* end of if(FrameFinished) */

		}/*  end  of vidoStream process  */


	/*----------------//////   process audio stream   \\\\\\\-----------------*/
		else if( audioStream >=0 && packet.stream_index==audioStream) { //only if audioStream exists
			//printf("processing audio stream...\n");
			/* bytes_used: indicates how many bytes of the data was consumed for decoding.
			         when provided with a self contained packet, it should be used completely.
			*  sb_size: hold the sample buffer size, on return the number of produced samples is stored.
			*/
			while(packet.size > 0) {
				//gettimeofday(&tm_start,NULL);
				bytes_used=avcodec_decode_audio4(aCodecCtx, pAudioFrame, &got_frame, &packet);
				//gettimeofday(&tm_end,NULL);
				//printf(" avcode_decode_audio4() cost time: %d ms\n",get_costtime(tm_start,tm_end) );
				if(bytes_used<0)
				{
					EGI_PDEBUG(DBG_FFPLAY,"Error while decoding audio! try to continue...\n");
					packet.size=0;
					packet.data=NULL;
					//av_free_packet(&packet);
					//break;
					continue;
				}
				/* if decoded data size >0 */
				if(got_frame)
				{
					//gettimeofday(&tm_start,NULL);
					/* playback audio data */
					if( !enable_audio_mono && pAudioFrame->data[0] && pAudioFrame->data[1])
					{
						// pAuioFrame->nb_sample = aCodecCtx->frame_size !!!!
						// Number of samples per channel in an audio frame
						//printf("Stereo.\n");
						if(sample_fmt == AV_SAMPLE_FMT_FLTP) {
							outsamples=swr_convert(swr,&outputBuffer, pAudioFrame->nb_samples, (const uint8_t **)pAudioFrame->extended_data, aCodecCtx->frame_size);
							//EGI_PDEBUG(DBG_FFPLAY,"outsamples=%d, frame_size=%d \n",outsamples,aCodecCtx->frame_size);
							egi_play_pcm_buff( (void **)&outputBuffer,outsamples);
						}
						else {
							 egi_play_pcm_buff( (void **)pAudioFrame->data, aCodecCtx->frame_size);// 1 frame each time
						}

					}
					else if(pAudioFrame->data[0]) {  /* one channel only */
						 //printf("Mono.\n");
						/* direct output */
						if(sample_fmt == AV_SAMPLE_FMT_FLTP) {
							outsamples=swr_convert(swr,&outputBuffer, pAudioFrame->nb_samples, (const uint8_t **)pAudioFrame->extended_data, aCodecCtx->frame_size);
							egi_play_pcm_buff( (void **)&outputBuffer,outsamples);
						}
						else {
						        egi_play_pcm_buff( (void **)(&pAudioFrame->data[0]), aCodecCtx->frame_size);// 1 frame each time
						}
					}

					/*    ---- 1024 points FFT displaying handling(pending) ---- */

					/* print audio playing time, only if no video stream */
					ff_sec_Aelapsed=atoi( av_ts2timestr(packet.pts,
				     			&pFormatCtx->streams[audioStream]->time_base) );
					ff_sec_Aduration=atoi( av_ts2timestr(pFormatCtx->streams[audioStream]->duration,
							&pFormatCtx->streams[audioStream]->time_base) );
/*
					printf("\r	     audio Elapsed time: %ds  ---  Duration: %ds  ",
								ff_sec_Aelapsed, ff_sec_Aduration );
*/

                                	/* --- Reset timing slider, if NO video stream ---- */
					if( videoStream < 0 )
						motpage_update_timingBar(ff_sec_Aelapsed, ff_sec_Aduration);

				}
				packet.size -= bytes_used;
				packet.data += bytes_used;
			}/* end of while(packet.size>0) */

		}/*   end of audioStream process  */

                /* free OLD packet each time, that was allocated by av_read_frame */
                av_free_packet(&packet);

/* For clip test, just ffplay a short time then break */
if(enable_clip_test)
{
		/* Check audio playing time */
		if( (audioStream >= 0) && (ff_sec_Aelapsed >= CLIP_PLAYTIME) )
		{
			/* reset timing */
			ff_sec_Aelapsed=0;
			ff_sec_Aduration=0;
			break;
		}
		/* If videoSteam or still pictures */
		else if( audioStream<0 )
		{
			/* reset timimg */
			ff_sec_Aelapsed=0;
			ff_sec_Aduration=0;

			tm_delayms(CLIP_PLAYTIME*1000);
			break;
		}
}

	/*----------------<<<<< Check and parse commands >>>>>-----------------*/
		if( FFmotion_Ctx->ffcmd != cmd_none )
		{
		    printf("%s:ffcmd received!\n",__func__);
		    /* 1. parse PAUSE/PLAY first */
		    if(FFmotion_Ctx->ffcmd==cmd_pause) {
			do {
				egi_sleep(0,0,100);
			} while(FFmotion_Ctx->ffcmd==cmd_pause); // !=cmd_play;

			/* Don not reset, pass down curretn cmd */

		    }
	  	    /* 2. shift mode */
		    else if( FFmotion_Ctx->ffcmd==cmd_mode) {
			    /* Note:
			     *      1. Default enable_filesloop is true.
			     *	    2. mode_loop_all means loop in order.
			     */
			    if(FFmotion_Ctx->ffmode==mode_repeat_one) {
				enable_seekloop=true;
				enable_shuffle=false;
			    }
			    else if(FFmotion_Ctx->ffmode==mode_shuffle) {
				enable_shuffle=true;
				enable_seekloop=false;
			    }
			    else if(FFmotion_Ctx->ffmode==mode_loop_all) {
				enable_seekloop=false;
				enable_shuffle=false;
			    }
			    /* reset */
	 		    FFmotion_Ctx->ffcmd=cmd_none;
		    }

		    /* 3. parse PREV/NEXT  */
		    else if(FFmotion_Ctx->ffcmd==cmd_next) {
		    	FFmotion_Ctx->ffcmd=cmd_none;
			//break;
			goto FAIL_OR_TERM;
		    }
		    else if(FFmotion_Ctx->ffcmd==cmd_prev) {
			FFmotion_Ctx->ffcmd=cmd_none;
			//printf("xxxxxxxxx  fnum=%d  xxxxxxx", fnum );
			if( fnum > 0 )
				fnum-=2;
			else
				fnum=-1;
			//break;
			goto FAIL_OR_TERM;
		    }

		    /* reset as cmd_none at last */
		    else
			FFmotion_Ctx->ffcmd=cmd_none;
		}

	   //printf("%s: restart av_read_frame()...\n",__func__);

	}/*  end of while()  <<---  end of one file playing by av_read_frame()  --->>  */




	/* hold on for a while, also let pic buff to be cleared before fbset_color!!! */
	if(LOOP_HOLDON_TIME>0)
	{
		/* NOTE: fnum may be illegal, as modified in ffcmd parsing, so skip to FAIL_OR_TERM! */
		EGI_PDEBUG(DBG_FFPLAY,"End playing %s, hold on for a while...\n",fpath[fnum]);
		tm_delayms(LOOP_HOLDON_TIME*1000);
	}

#if 0 /* These codes may be skipped, move to FAIL_OR_TERM */
	/* fill display area with BLACK */
	fbset_color(WEGI_COLOR_BLACK);
	//draw_filled_rect(&ff_fb_dev, pic.Hs ,pic.Vs, pic.He, pic.Ve); /* pic area */
	draw_filled_rect(&ff_fb_dev, 0,30, 239,265); /* display zone */
#endif

/* if loop playing one file, then got to seek start  */
if(enable_seekloop)
{
	EGI_PDEBUG(DBG_FFPLAY,"enable_seekloop=true, go to seek the start of the same file and replay. \n");
	goto SEEK_LOOP_START;
}

FAIL_OR_TERM:
	/* fill display area with BLACK
	 * NOTE: As thread audioSpectrum is still working for a while after draw_filled_rect() here,
	 * 	 it's NOT the right itme to clear screen!
	 */
#if 0
	fbset_color(WEGI_COLOR_BLACK);
	draw_filled_rect(&ff_fb_dev, 0,30, 239,265); /* display zone */
#endif


	/*  <<<<<<<<<<  start to release all resources  >>>>>>>>>>  */


	//if(videoStream >=0 && pthd_displayPic_running==true ) /* only if video stream exists */
	if( pthd_displayPic_running==true )
	{
		/* wait for display_thread to join */
		EGI_PDEBUG(DBG_FFPLAY,"Try to join picture and subtitle displaying thread ...\n");
		/* give a command to exit display_thread, before exiting subtitle_thread!! */
		control_cmd = cmd_exit_display_thread;
		pthread_join(pthd_displayPic,NULL);

		if(pthd_subtitle_running) {
			EGI_PDEBUG(DBG_FFPLAY,"Try to join subtitle displaying thread ...\n");
	                control_cmd = cmd_exit_subtitle_thread;
			pthread_join(pthd_displaySub,NULL);  /* Though it will exit when reaches end of srt file. */
			pthd_subtitle_running=false; /* reset token */
		}

		control_cmd = cmd_none;/* call off command */
		pthd_displayPic_running=false; /* reset token */
	}

	/* free strdupped file name, after pthd_displayPic()  */
	free(fname); fname=NULL;

	/* Free the YUV frame */
	EGI_PDEBUG(DBG_FFPLAY,"Free pFrame...\n");
	if(pFrame != NULL) {
		av_frame_free(&pFrame);
		pFrame=NULL;
		EGI_PDEBUG(DBG_FFPLAY,"	...pFrame freed.\n");
	}

	/* Free pAudioFrame */
	if(pAudioFrame != NULL) {
		av_frame_free(&pAudioFrame);
		pAudioFrame=NULL;
		EGI_PDEBUG(DBG_FFPLAY,"	...pAudioFrame freed.\n");
	}

	/* Free the RGB image */
	EGI_PDEBUG(DBG_FFPLAY,"free buffer...\n");
	if(buffer != NULL) {
		av_free(buffer);
		buffer=NULL;
		EGI_PDEBUG(DBG_FFPLAY,"	...buffer freed.\n");
	}

	EGI_PDEBUG(DBG_FFPLAY,"Free pFrameRGB...\n");
	if(pFrameRGB != NULL) {
		av_frame_free(&pFrameRGB);
		pFrameRGB=NULL;
		EGI_PDEBUG(DBG_FFPLAY,"	...pFrameRGB freed.\n");
	}

	/* close pcm device and audioSpectrum */
//	if(audioStream >= 0) {
		EGI_PDEBUG(DBG_FFPLAY,"Close PCM device...\n");
		egi_close_pcm_device();

		/* exit audioSpectrum thread(pending) */
//	}

	/* free outputBuffer */
	if(outputBuffer != NULL)
	{
		EGI_PDEBUG(DBG_FFPLAY,"Free outputBuffer for pcm...\n");
		free(outputBuffer);
		outputBuffer=NULL;
	}

if(enable_avfilter) /* free filter resources */
{
	/* free filter items */
	if(filter_graph != NULL)
	{
		EGI_PDEBUG(DBG_FFPLAY,"avfilter graph free ...\n");
		/* It will also free all AVFilters in the filter graph */
		avfilter_graph_free(&filter_graph);
		filter_graph=NULL;
	}
	/* Free filted frame */
	EGI_PDEBUG(DBG_FFPLAY,"Free filt_pFrame...\n");
	if(filt_pFrame != NULL) {
		av_frame_free(&filt_pFrame);
		filt_pFrame=NULL;
	}
}

	/* Close the codecs */
	EGI_PDEBUG(DBG_FFPLAY,"Closee the codecs...\n");
	avcodec_close(pCodecCtx);
	pCodecCtx=NULL;
	avcodec_close(pCodecCtxOrig);
	pCodecCtxOrig=NULL;
	avcodec_close(aCodecCtx);
	aCodecCtx=NULL;
	avcodec_close(aCodecCtxOrig);
	aCodecCtxOrig=NULL;


	/* Close the video file */
	EGI_PDEBUG(DBG_FFPLAY,"avformat_close_input()...\n");
	if(pFormatCtx != NULL) {
		avformat_close_input(&pFormatCtx);
		pFormatCtx=NULL;
	}

        /* free buff for subtitle fpath */
        if( pfsub != NULL ) {
		EGI_PDEBUG(DBG_FFPLAY,"Free pfsub of subtitle fpath...\n");
                free(pfsub);
                pfsub=NULL;
        }

//	if(audioStream >= 0)
//	{
		EGI_PDEBUG(DBG_FFPLAY,"Free swr at last...\n");
		swr_free(&swr);
		swr=NULL;
//	}

//	if(videoStream >= 0)
//	{
		EGI_PDEBUG(DBG_FFPLAY,"Free sws_ctx at last...\n");
		sws_freeContext(sws_ctx);
		sws_ctx=NULL;
//	}


	/* print total playing time for the file */
	gettimeofday(&tm_end,NULL);
	EGI_PDEBUG(DBG_FFPLAY,"Playing %s cost time: %d ms\n",fpath[fnum_playing],
									tm_signed_diffms(tm_start,tm_end) );

	EGI_PDEBUG(DBG_FFPLAY," \n ---( End of playing file %s )--- \n",fpath[fnum_playing]);

	/* sleep, to let sys release cache ....???? */
	tm_delayms(END_PAUSE_TIME);

   } /* end of for(...), loop playing input files*/


   //tm_delayms(500);

  /* --- PLAY ALL FILES END --- */
  if(!enable_filesloop) /* if disable loop playing all files, then break here */
  {
	break;
  }
  EGI_PDEBUG(DBG_FFPLAY," \n----( End of Playing One Complete Round )--- \n\n\n\n\n");

} /* end of while(1), eternal loop. */


	/* close fb dev */
	release_fbdev(&ff_fb_dev);

        return 0;
}
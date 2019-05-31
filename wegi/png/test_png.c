/*--------------------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

A simple test for displaying PNG files.

Note:
1. color_type,	Bit depth,	 Description
   0		1,2,4,8,16 	 each pixel is a grayscale sample
   2		8,16		 each pixel is an RGB triple.
   3		1,2,4,8		 each pixel is a platte index
   4		8,16		 a grayscale pixel followed by an alpha sample
   6		8,16		 a RGB triple pixel followed by an alpha sample
   Referring to: https//www.w3.org/TR/PNG/
2. Input image is transformed to RGB 24bpp type by setting option PNG_TRANSFORM_EXPAND
   in png_read_png().
3.


Midas Zhou
midaszhou@yahoo.com
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "egi_fbgeom.h"
#include "egi_bjp.h"
#include "egi_image.h"
#include "egi_color.h"

#define PNG_FIXED_POINT_SUPPORTED /* for read */
#define PNG_READ_EXPAND_SUPPORTED

int main(int argc, char **argv)
{
	int ret=0;
	int n;

	EGI_IMGBUF  eimg={0};

	if( argc<2 ) {
		printf("Please enter png file name!\n");
		return -1;
	}

        /* --- prepare fb device --- */
        init_fbdev(&gv_fb_dev);

 /* display all png files in input list */
 for(n=1; n<argc; n++) {
	printf("	----- %d/%d ---- \n",n,argc-1);

	if( egi_imgbuf_loadpng(argv[n], &eimg ) ==0 ) {
	}
	else if( egi_imgbuf_loadjpg(argv[n], &eimg ) ==0 ) {
	}
	else {
		/* loop ... */
		if(n==(argc-1))
			n=0;
		continue;
	}


        /* window_position displaying */
#if 1
	int dw,dh; /* displaying window width and height */
	dw=eimg.width>240?240:eimg.width;
	dh=eimg.height>320?320:eimg.height;
        egi_imgbuf_windisplay(&eimg, &gv_fb_dev, 0, 0, 0, 0, dw, dh);
#else
        egi_imgbuf_windisplay(&eimg, &gv_fb_dev, 0, 0, 70, 220, eimg.width, eimg.height);
#endif
	sleep(1);
	egi_imgbuf_release(&eimg);


	/* loop ... */
	if(n==(argc-1))
		n=0;

  } /* end of for() */

	release_fbdev(&gv_fb_dev);

	return ret;
}
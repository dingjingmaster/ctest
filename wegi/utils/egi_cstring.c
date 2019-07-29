/*-----------------------------------------------------------------
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.


Char and String Functions

Midas Zhou
----------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "egi_cstring.h"
#include "egi_log.h"


/*------------------------------------------------------------------
Duplicate a file path string, then replace the extension name
with the given one.

fpath:		a file path string.
new_extname:	new extension name for the fpath.

NOTE:
	1. The old extension name MUST have only one '.' char.
	   or the last '.' will be deemed as start of extension name!
	2. The new_extname MUST start with '.' !
	3. Remember to free the new_fpath later after call!

Return:
	pointer	to new string	OK
        NULL			Fail, or not found.
-------------------------------------------------------------------*/
char * cstr_dup_repextname(char *fpath, char *new_extname)
{
	char *pt=NULL;
	char *ptt=NULL;
	char *new_fpath;
	int  len,fplen;

	if(fpath==NULL || new_extname==NULL)
	return NULL;

	pt=strstr(fpath,".");
	if(pt==NULL)
		return NULL;

	/* Make sure its the last '.' in fpath */
	for( ; ( ptt=strstr(pt+1,".") ) ; pt=ptt)
	{ };

	/* duplicate and replace */
	len= (long)pt - (long)fpath +1;
	fplen= len + strlen(new_extname) +2;

	new_fpath=calloc(1, fplen*sizeof(char));
	if(new_fpath==NULL)
		return NULL;

	snprintf(new_fpath,len,fpath);
	strcat(new_fpath,new_extname);

	return new_fpath;
}



/*--------------------------------------------------------
Get pointer to the first char after Nth split char.

str:	source string.
split:  split char (or string).
n:	the n_th split
	if n=0, then return split=str;

Example str:"data,12.23,94,343.43"
	split=",", n=2 (from 0...)
	then get pointer to '9',

Return:
	pointer	to a char	OK, spaces trimed.
        NULL			Fail, or not found.
---------------------------------------------------------*/
char * cstr_split_nstr(char *str, char *split, unsigned n)
{
	int i;
	char *pt;

	if(str==NULL || split==NULL)
		return NULL;

	if(n==0)return str;

	pt=str;
	for(i=1;i<=n;i++) {
		pt=strstr(pt,split);
		if(pt==NULL)return NULL;
		pt=pt+1;
	}

	return pt;
}


/*--------------------------------------------------
Trim all spaces at end of a string, return a pointer
to the first non_space char.
Return:
	pointer	to a char	OK, spaces trimed.
        NULL			Input buf invalid
--------------------------------------------------*/
char * cstr_trim_space(char *buf)
{
	char *ps, *pe;

	if(buf==NULL)return NULL;

	/* skip front spaces */
	ps=buf;
	for(ps=buf; *ps==' '; ps++)
	{ };

	/* eat up back spaces/returns, replace with 0 */
	for(  pe=buf+strlen(buf)-1;
	      *pe==' '|| *pe=='\n' || *pe=='\r' ;
	      (*pe=0, pe-- ) )
	{ };

	/* if all spaces, or end token
	 * if ps==pe, means there is only ONE char in the line.
	*/
	if( (long)ps > (long)pe ) { //ps==pe || *ps=='\0' ) {
//		printf("%s: Input line buff contains all but spaces. ps=%p, pe=%p \n",
//					__func__,ps,pe);
			return NULL;
	}

	//printf("%s: %s, [0]:%c, [9]:%c, [10]:%c\n",__func__,ps,ps[0],ps[9],ps[10]);
	return ps;
}



/*-----------------------------------------------------------------------
Get length of a character with UTF-8 encoding.

@cp:	A pointer to a character with UTF-8 encoding.

Note:
1. UTF-8 maps to UNICODE according to following relations:
				(No endianess problem for UTF-8 conding)

	   --- UNICODE ---	      --- UTF-8 CODING ---
	U+  0000 - U+  007F:	0XXXXXXX
	U+  0080 - U+  07FF:	110XXXXX 10XXXXXX
	U+  0800 - U+  FFFF:	1110XXXX 10XXXXXX 10XXXXXX
	U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX

2. If illegal coding is found...

Return:
	>0	OK, string length in bytes.
	<0	Fails
------------------------------------------------------------------------*/
inline int cstr_charlen_uft8(const unsigned char *cp)
{
	if(cp==NULL)
		return -1;

	if(*cp < 0b10000000)
		return 1;	/* 1 byte */
	else if( *cp >= 0b11110000 )
		return 4;	/* 4 bytes */
	else if( *cp >= 0b11100000 )
		return 3;	/* 3 bytes */
	else if( *cp >= 0b11000000 )
		return 2;	/* 2 bytes */
	else
		return -2;	/* unrecognizable starting byte */
}



/*-----------------------------------------------------------------------
Get total number of characters in a string with UTF-8 encoding.

@cp:	A pointer to a string with UTF-8 encoding.

Note:
1. Count all ASCII characters, both printables and unprintables.

2. UTF-8 maps to UNICODE according to following relations:
				(No endianess problem for UTF-8 conding)

	   --- UNICODE ---	      --- UTF-8 CODING ---
	U+  0000 - U+  007F:	0XXXXXXX
	U+  0080 - U+  07FF:	110XXXXX 10XXXXXX
	U+  0800 - U+  FFFF:	1110XXXX 10XXXXXX 10XXXXXX
	U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX


3. If illegal coding is found...

Return:
	>=0	OK, total numbers of character in the string.
	<0	Fails
------------------------------------------------------------------------*/
int cstr_strcount_uft8(const unsigned char *pstr)
{
	unsigned char *cp;
	int size;	/* in bytes, size of the character with UFT-8 encoding*/
	int count;

	if(pstr==NULL)
		return -1;

	cp=(unsigned char *)pstr;
	count=0;
	while(*cp) {
		size=cstr_charlen_uft8(cp);
		if(size>0) {
			//printf("%d\n",*cp);
			count++;
			cp+=size;
			continue;
		}
		else {
		   printf("%s: WARNGING! unrecognizable starting byte for UFT-8. Try to skip it.\n",__func__);
		   	cp++;
		}
	}

	return count;
}


/*----------------------------------------------------------------------
Convert a character from UFT-8 to UNICODE.

@src:	A pointer to characters with UFT-8 encoding.
@dest:	A pointer to mem space to hold converted characters with UNICODE.
	The caller shall allocate enough space for dest.
	!!!!TODO: Dest is now supposed to be in little-endian ordering. !!!!
@n:	Number of characters expected to be converted.

1. UTF-8 maps to UNICODE according to following relations:
				(No endianess problem for UTF-8 conding)

	   --- UNICODE ---	      --- UTF-8 CODING ---
	U+  0000 - U+  007F:	0XXXXXXX
	U+  0080 - U+  07FF:	110XXXXX 10XXXXXX
	U+  0800 - U+  FFFF:	1110XXXX 10XXXXXX 10XXXXXX
	U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX


2. If illegal coding is found...

Return:
	>0 	OK, bytes of src consumed and converted into unicode.
	<0	Fails
-----------------------------------------------------------------------*/
inline int char_uft8_to_unicode(const unsigned char *src, wchar_t *dest)
{
	unsigned char *cp; /* tmp to dest */
	unsigned char *sp; /* tmp to src  */

	int size;	/* in bytes, size of the character with UFT-8 encoding*/

	if(src==NULL || dest==NULL )
		return -1;

	cp=(unsigned char *)dest;
	sp=(unsigned char *)src;

	size=cstr_charlen_uft8(src);

	if(size<0) {
		return size;	/* ERROR */
	}

	/* U+ 0000 - U+ 007F:	0XXXXXXX */
	else if(size==1) {
		*dest=0;
		*cp=*src;	/* The LSB of wchar_t */
	}

	/* U+ 0080 - U+ 07FF:	110XXXXX 10XXXXXX */
	else if(size==2) {
		/*dest=0;*/
		*dest= (*(sp+1)&0x3F) + ((*sp&0x1F)<<6);
	}

	/* U+ 0800 - U+ FFFF:	1110XXXX 10XXXXXX 10XXXXXX */
	else if(size==3) {
		*dest= (*(sp+2)&0x3F) + ((*(sp+1)&0x3F)<<6) + ((*sp&0xF)<<12);
	}

	/* U+ 10000 - U+ 1FFFF:	11110XXX 10XXXXXX 10XXXXXX 10XXXXXX */
	else if(size==4) {
		*dest= (*(sp+3)&0x3F) + ((*(sp+2)&0x3F)<<6) +((*(sp+2)&0x3F)<<12) + ((*sp&0x7)<<18);
	}

	return size;
}





/*----------------------------------------------------------------------------------
Search given SECTION and KEY string in the config file, copy VALUE
string to the char *value if found.

sect:	Char pointer to a given SECTION name.
key:	Char pointer to a given KEY name.
pvalue:	Char pointer to a char buff that will receive found VALUE string.

NOTE:
1. A config file should be edited like this:
# comment comment comment
# comment comment commnet
        # comment comment
  [ SECTION1]
KEY1 = VALUE1 #  !!!! all chars after '=' (including '#' and this comment) will be parsed as value of KEY2 !!!
KEY2= VALUE2

##########   comment
	####comment
##
[SECTION2 ]
KEY1=VALUE1
KEY2= VALUE2
...

1. Lines starting with '#' are deemed as comment lines.
2. Lines starting wiht '[' are deemed as start/end/boundary of a SECTION.
3. Non_comments lines containing a '=' are parsed as assignment for KEYs with VALUEs.
4. All spaces beside SECTION/KEY/VALUE strings will be ignored/trimmed.
5. If there are more than one section with the same name, only the first
   one is valid, and others will be all neglected.
6. 
		[[ ------  LIMITS -----  ]]
6. Max. length for one line in a config file is 256-1. ( see. line_buff[256] )
7. Max. length of a SECTION/KEY/VALUE string is 64-1. ( see. str_test[64] )

Return:
	3	VALE string is NULL
	2	Fail to find KEY string
	1	Fail to find SECTION string
	0	OK
	<0	Fails
------------------------------------------------------------------------------------*/
int egi_get_config_value(char *sect, char *key, char* pvalue)
{
	int lnum=0;
	int ret=0;

	FILE *fil;
	char line_buff[256];
	char *ps=NULL, *pe=NULL; /* start/end char pointer for [ and  ] */
	char *pt=NULL;
	char str_test[64];
	int  found_sect=0; /* 0 - section not found, !0 - found */

	/* check input param */
	if(sect==NULL || key==NULL || pvalue==NULL) {
		printf("%s: One or more input param is NULL.\n",__func__);
		return -1;
	}

	/* open config file */
	fil=fopen( EGI_CONFIG_PATH, "re");
	if(fil==NULL) {
		printf("Fail to open config file '%s', %s\n",EGI_CONFIG_PATH, strerror(errno));
		return -2;
	}
	//printf("Succeed to open '%s', with file descriptor %d. \n",EGI_CONFIG_PATH, fileno(fil));

	fseek(fil,0,SEEK_SET);

	/* Search SECTION and KEY line by line */
	while(!feof(fil))
	{
		lnum++;

		memset(line_buff,0,sizeof(line_buff));
		fgets(line_buff,sizeof(line_buff),fil);
		//printf("line_buff: %s\n",line_buff);

		/* 0. cut the return key '\r' '\n' at end .*/
		/*   OK, cstr_trim_space() will do it */

		/* 1.  Search SECTION name in the line_buff */
		if(!found_sect)
		{
			/* Ignore comment lines starting with '#' */
			ps=cstr_trim_space(line_buff);
			/* get rid of all_space/empty line */
			if(ps==NULL) {
//				printf("config file:: Line:%d is an empty line!\n", lnum);
				continue;
			}
			else if( *ps=='#') {
//				printf("Comment: %s\n",line_buff);
				continue;
			}
			/* search SECTION name between '[' and ']'
			 * Here we assume that [] only appears in a SECTION line, except comment line.
			 */
			ps=strstr(line_buff,"[");
			pe=strstr(line_buff,"]");
			if( ps!=NULL && pe!=NULL && pe>ps) {
				memset(str_test,0,sizeof(str_test));
				strncpy(str_test,ps+1,pe-ps+1-2); /* strip '[' and ']' */
				//printf("SECTION: %s\n",str_test);

				/* check if SECTION name matches */
				if( strcmp(sect,cstr_trim_space(str_test))==0 ) {
					printf("Found SECTION:[%s] \n",str_test);
					found_sect=1;
				}
			}
		}
		/* 2. Search KEY name in the line_buff */
		else /* IF found_sect */
		{
			ps=cstr_trim_space(line_buff);
			/* bypass blank line */
			if( ps==NULL )continue;
			/* bypass comment line */
			else if( *ps=='#' ) {
//				printf("Comment: %s\n",line_buff);
				continue;
			}
			/* If first char is '[', it reaches the next SECTION, so give up. */
			else if( *ps=='[' ) {
				printf("Get bottom of the section, fail to find key '%s'.\n",key);
				ret=2;
				break;
			}

			/* find first '=' */
			ps=strstr(line_buff,"=");
			/* assure NOT null and '=' is NOT a starting char 
			 * But, we can not exclude spaces before '='.
			 */
			if( ps!=NULL && ps != line_buff ) {
				memset(str_test,0,sizeof(str_test));
				/* get key name string */
				strncpy(str_test, line_buff, ps-line_buff);
//				printf(" key str_test: %s\n",str_test);
				/* assure key name is not NULL */
				if( (ps=cstr_trim_space(str_test))==NULL) {
				   printf("Key name is NULL in line_buff: '%s' \n",line_buff);
				   continue;
				}
				/* if key name matches */
				else if( strcmp(key,ps)==0 ) {
					//printf("found KEY:%s \n",str_test);
					ps=strstr(line_buff,"="); /* !!! again, point ps to '=' */
					pt=cstr_trim_space(ps+1);
					/* Assure the value is NOT null */
					if(pt==NULL) {
					   printf("%s: Value of key [%s] is NULL in line_buff: '%s' \n",
										__func__, key, line_buff);
					   ret=3;
					   break;
					}
					/* pass VALUE to pavlue */
					else {
					   strcpy(pvalue, pt);
					   printf("%s: Found  Key:[%s],  Value:[%s] \n",
										__func__, key,pvalue);
					   ret=0; /* OK, get it! */
					   break;
					}
				}
			}
		}

	} /* end of while() */

	if(!found_sect) {
		printf("%s: Fail to find given SECTION:[%s] in config file.\n",__func__,sect);
		ret=1;
	}
#if 1
	/* log errors */
	if(ret !=0 ) {
		EGI_PLOG(LOGLV_ERROR,"%s: Fail to get value of key:[%s] in section:[%s] in config file.\n",
										      __func__, key, sect);
	}
	else {
		EGI_PLOG(LOGLV_CRITICAL,"%s: Get value of key:[%s] in section:[%s] in config file.\n",
										      __func__, key, sect);
	}
#endif
	fclose(fil);
	return ret;
}

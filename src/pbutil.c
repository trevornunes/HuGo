/*
 * pbutil.c
 *  ROM file navigator
 *  Created on: Jan 19, 2012
 *      Author: tnunes
 */

#include <stdio.h>
#include <dirent.h>
#include "pbutil.h"


/*
 *  The purpose of this module is to encapsulate the management of ROM files.
 *  It provides a way of iterating through files in a directory and returning the name of each file
 *  You can query current, increment, decrement an index.  The intent is to decouple the ROM navigation from
 *  the UI of Emulators and have this common code used with a simple string display and up,down,select style
 *  interface.  For now it's just internal code calls that give an emulator a rom cycling capabality ( getting a ROM one at time in dirent order)
 *
 *  TODO:  Dynamic allocate string array for names using romEntry database
 *  TODO:  Sort data by name.  ( would be a lot easy with vector strings ,but I need this to be pure C for portability with some emulators )
 *
 *  RomSelectStart();           // initialize
 *
 *  char *romName = RomNext();  // get first ROM file in directory
 *
 *  romName = RomNext();        // get second rom file
 *  romName = RomNext();
 *  // You can call RomNext forever it will reset back  0.1.2.3.4.5....0.1.2.3.4.5 etc.
 *  romName = RomSelectDown();  // get previous rom file
 *  romName = RomSelectUp();    // get next rom file after current index.
 *  romName = RomNameAtIndex(5); // get the name of the ROM that is the 5th file in the list.
 *
 *
 */


char       g_runningFile_str[64];
#define    PB_ROM_PATH "/accounts/1000/shared/misc/roms/pce"
static int RomCurrentIndex = 0;
static int RomCount = 0;

typedef struct romEntry {
  char name[32];
  int  valid;
} romEntry_t;

romEntry_t *ROM_LIST;


/*
 *
 */
/* name: RomSelectStart
 * description: reset index to zero, get number of roms in PB_ROM_PATH
 *              ROM navigate to next item in the ROM list
 *              TODO: allocate and build rom file listing array.
 * parameters:  none
 * returns:     none
 *
 */
void RomSelectStart(void)
{
	RomCurrentIndex =0;
	RomCount = RomCountInDir( PB_ROM_PATH );
	fprintf(stderr,"RomSelectStart: %d roms found\n", RomCount);
  //  ROM_LIST = (romEntry_t *) malloc( sizeof(romEntry_t) * RomCount);
}


/*
 *
 */
/* name: RomGetCurrentIndex
 * description: return the current index counter.
 * parameters:  none
 * returns:     Current index value
 *
 */
int RomGetCurrentIndex(void)
{
	return RomCurrentIndex;
}


/* name: RomSelectUp
 * description: ROM navigate to next item in the ROM list
 *              each call iterates the RomCurrentIndex counter
 *              returns the rom at the index selected. Will continue
 *              to iterate until a valid file is returned > 4 characters
 *              or returns an empty string if it fails.
 * parameters:  none
 * returns:     const char * name of file at index, minus the path.
 *
 */
const char *RomSelectUp(void)
{
	char *p = 0;

	if ( RomCurrentIndex++ >= RomCount )
	 	 RomCurrentIndex = 0;

    p = RomNameAtIndex( PB_ROM_PATH, RomCurrentIndex);

    while( strlen(p) <= 3)
    {
    	fprintf(stderr,"RomSelectUp: invalid rom name ...\n");
    	p = RomNameAtIndex( PB_ROM_PATH, RomCurrentIndex++);
    	if( RomCurrentIndex >= RomCount)
    		RomCurrentIndex = 0;
    	    return "";
    }
    if(!p)
    {
      fprintf(stderr,"RomSelectUp: Got an empty string from RomNameAtIndex ..\n");
    }

    return p;
}



/* name: RomSelectDown
 * description: ROM navigate to up list to previous item
 *              each call decrements the ROM index counter.
 *              It's the compliment too RomSelectUp.
 * parameters:  none
 * returns:     const char * name of file at index, minus the path.
 *
 */
const char *RomSelectDown(void)
{
	if( RomCurrentIndex-- <= 0)
		RomCurrentIndex = 0;

	return( RomNameAtIndex( PB_ROM_PATH, RomCurrentIndex) );
}

/*
 *
 */
int RomCountInDir( const char *dpath )
{
  DIR* dirp;
  struct dirent* direntp;
  int fileCount = 0;

  if(!dpath)
  {
    fprintf(stderr,"dpath is null.\n");
    return 0;
  }
  dirp = opendir( dpath);
  if( dirp != NULL )
  {
	 for(;;)
	 {
		direntp = readdir( dirp );
		if( direntp == NULL)
		    break;

		  if( strcmp( direntp->d_name, ".") == 0)
			 continue;

		  if( strlen(direntp->d_name) <= 4)
			  continue;

		  if(strcmp(direntp->d_name,"..") == 0)
			  continue;

	     // fprintf(stderr,"ROM: %s\n", direntp->d_name);
	      fileCount++;
	 }
  }

  closedir(dirp);
  return fileCount;
}

/*
 *
 */
const char *RomNameAtIndex( char *dpath, int index )
{
  DIR* dirp;
  FILE* fp;

  struct dirent* direntp;
  int fileCount = 0;

  if(!dpath)
  {
    fprintf(stderr,"RomNameAtIndex: directory path is NULL\n");
    return 0;
  }

  dirp = opendir( dpath);

  if( dirp != NULL )
  {
	 for(;;)
	 {
		direntp = readdir( dirp );
		if( direntp == NULL)
		    break;

		  if( strcmp( direntp->d_name, ".") == 0)
			 continue;

		  if( strlen(direntp->d_name) <= 4)
			  continue;

		  if(strcmp(direntp->d_name,"..") == 0)
			  continue;

	     // fprintf(stderr,"RomNameAtIndex %d is '%s'\n", index, direntp->d_name);

	      if(fileCount++ == index) {
	    	  fprintf(stderr,"ROM: %s\n", direntp->d_name);
	          closedir(dirp);
	          return direntp->d_name;
	      }
	 }
  }

  closedir(dirp);
  return "";
}




/* name: RomNext
 * description: ROM cycler.
 *              Iterate through directory listing and return the next rom name in list
 *              if the iteration >= the maximum list in directory it just resets back.
 * parameters:  none
 * returns:     const char * name of file at index, minus the path.
 *
 */
const char* RomNext(void)
{
    int status = 0;
    int count = RomCount;
    char *romName = 0;
    static int badFileCounter;

    if(RomCount == 0)
       return 0;


    romName = RomSelectUp();
    while ( strlen(romName) <= 4)
    {
      if( badFileCounter++ > 10)
      {
    	  fprintf(stderr,"RomNext: 10 bad files found something is wrong or directory contains some odd file names\n");
          return "NO_VALID_FILE";
      }

      fprintf(stderr,"RomNext: [%02d] bad file?\n", RomGetCurrentIndex() );
      romName = RomSelectUp();
    }

    memset(&g_runningFile_str[0],0,64);
    sprintf(&g_runningFile_str[0], romName );

    fprintf(stderr,"RomNext: selected '%s' index=%d/%d\n", romName, RomGetCurrentIndex(), RomCount );
    return romName;
}

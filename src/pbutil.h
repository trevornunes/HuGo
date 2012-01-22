/*
 * pbutil.h
 *
 *  Created on: Jan 19, 2012
 *      Author: tnunes
 */

#ifndef PBUTIL_H_
#define PBUTIL_H_


        int RomCountInDir( const char *dpath );        /* count how many files,roms in the directory. */
const char *RomNameAtIndex( char *dpath, int index );  /* get rom name at supplied index */
       void RomSelectStart(void);   /* get rom count in dir, reset index to zero */
const char *RomSelectUp(void);      /* iterate rom index up return name of rom at this index */
const char *RomSelectDown(void);    /* iterate rom index down return name of rom at this index */
        int RomGetCurrentIndex(void);   /* return rom name at current index */
const char* RomNext(void);              /* get next rom name in list, will auto reset to 0 ... */


#endif /* PBUTIL_H_ */

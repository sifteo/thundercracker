/**************************************************************************
 * SCZ.h - Function protoypes for User-Callable SCZ compression routines. *
 *  This file is useful to #include within applications that link SCZ     *
 *  functions, when SCZ library routines are pre-compiled into .o files.  *
 * SCZ-version 1.8	11-25-08					  *
 **************************************************************************/

#ifndef SCZ_DEFS
#define SCZ_DEFS 1

#define SCZ_VERSION 1.8
float scz_version=SCZ_VERSION;

/************************************************************************/
/* Scz_Compress_File - Compresses input file to output file.		*/
/*  First argument is input file name.  Second argument is output file	*/
/*  name.  The two file names must be different.			*/
/************************************************************************/
int Scz_Compress_File( char *infilename, char *outfilename );


/************************************************************************/
/* Scz_Compress_Buffer2File - Compresses character array input buffer	*/
/*  to an output file.  This routine is handy for applications wishing	*/
/*  to compress their output directly while saving to disk.		*/
/*  First argument is input array, second is the array's length, and	*/
/*  third is the output file name to store to.				*/
/************************************************************************/
int Scz_Compress_Buffer2File( unsigned char *buffer, int N, char *outfilename );


/************************************************************************/
/* Scz_Compress_Buffer2Buffer - Compresses character array input buffer	*/
/*  to another buffer.  This routine is handy for applications wishing	*/
/*  to compress data on the fly, perhaps to reduce dynamic memory or	*/
/*  network traffic between other applications.				*/
/*  First argument is input array, second is the array's length, the	*/
/*  third is the output buffer array, with return variable for array	*/
/*  length, and the final parameter is a continuation flag.  Set true	*/
/*  if last segment, otherwise set false if more segments to come.	*/
/*  This routine allocates the output array and passes back the size.	*/ 
/************************************************************************/
int Scz_Compress_Buffer2Buffer( char *inbuffer, int N, char **outbuffer, int *M, int lastbuf_flag );


/************************************************************************/
/* Scz_Decompress_File - Decompresses input file to output file.        */
/*  First argument is input file name.  Second argument is output file  */
/*  name.  The two file names must be different.                        */
/************************************************************************/
int Scz_Decompress_File( char *infilename, char *outfilename );


/***************************************************************************/
/* Scz_Decompress_File2Buffer - Decompresses input file to output buffer.  */
/*  This routine is handy for applications wishing to read compressed data */
/*  directly from disk and to uncompress the input directly while reading  */
/*  from the disk.             						   */
/*  First argument is input file name.  Second argument is output buffer   */
/*  with return variable for array length. 				   */
/*  This routine allocates the output array and passes back the size.      */ 
/**************************************************************************/
int Scz_Decompress_File2Buffer( char *infilename, char **outbuffer, int *M );


/*******************************************************************************/
/* Scz_Decompress_Buffer2Buffer - Decompresses input buffer to output buffer.  */
/*  This routine is handy for decompressing data on the fly, to reduce dynamic */
/*  memory or network traffic between applications.    			       */
/*  First argument is input array, second is the array's length, the third is  */
/*  the output buffer array, with return variable for array length.  	       */
/*  This routine allocates the output array and passes back the size.          */ 
/*******************************************************************************/
int Scz_Decompress_Buffer2Buffer( char *inbuffer, int N, char **outbuffer, int *M );


void scz_cleanup();	/* Call after last SCZ call to free temporarily allocated memory. */


#endif

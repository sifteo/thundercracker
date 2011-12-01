SCZ - Simple Compression/Decompression Routines and Utilities:
--------------------------------------------------------------

Simple light-weight file compression and decompression utilities.
Written in C and portable to all platforms.  Simple compress and
decompress (SCZ) subroutines can be linked and called within
applications to compress or unpack data on the fly, or as stand-alone
utilities.  This initial set of routines implement a new lossless 
compression algorithm under LGPL license.  Restoration (decompression)
is perfect.  


Contents:

  Readme.txt  - This file.

  scz.h	      - Function protoypes for linking to SCZ routines.

  scz_core.c  - Common internal routines & variables included by all SCZ files.

  scz_compress_lib.c   - Base compression functions.
	These routines are to be included and called from other programs.
	This file contains the following user-callable convenience routines:
	  Scz_Compress_File( infilename, outfilename )
	  Scz_Compress_Buffer2File( buffer, N, outfilename )
	  Scz_Compress_Buffer2Buffer(inbuffer,N, outbuffer,M,lastbuf_flag)
	(See comments above each routine for usage instructions and formal defs.)

  scz_decompress_lib.c - Base decompression functions.  These functions reverse
	what the scz_compress_lib.c do.  They are to be included and called 
	from other programs. 
        This file contains the following user-callable convenience routines:
	  Scz_Decompress_File( *infilename, *outfilename );
	  Scz_Decompress_File2Buffer( *infilename, **outbuffer, *M );
	  Scz_Decompress_Buffer2Buffer( *inbuffer, *N, **outbuffer, *M );
	(See comments above each routine for usage instructions and formal defs.)

  scz_streams.c - Alternative stream-oriented methods for accessing scz-comp/decomp.
	Write or read individual lines or XML tags on an incremental basis,
	instead of whole buffers or files in one call.  File contains:
	  Scz_File_Open( filename, mode );
	  Scz_Feof( sczfile );
	  Scz_ReadString( sczfile, delimiter, outstring, maxN );
	  Scz_WriteString( sczfile, string, N );
	  Scz_File_Close( sczfile );

  scz_compress.c   - Stand-alone file compression utility, uses scz_compress_lib.c.
	Compile:   cc -O scz_compress.c -o scz_compress

  scz_decompress.c - Stand-alone file decompression utility, uses scz_decompress_lib.c.
	Compile:   cc -O scz_decompress.c -o scz_decompress



Intro:
SCZ is intended as a simple routine for calling from within other applications
without legal or technical encumberances.  Developed because other compression 
routines, such as gzip, Zlib, etc., are fairly large, complex, difficult to 
integrate, maintain, understand, and are often larger than many applications 
themselves.  SCZ addresses the nitch for a simple lightweight data-compress/decompress
routine that can be included within other applications, permitting applications
to compress or decompress data on-the-fly, during read-in or write-out by
a simple call.  SCZ often achieves 3:1 compression.  On binary PPM image files
it may achieve a 10:1 compression.  On some text files, I have seen 25:1.
On difficult files, it may achieve less-than 2:1 reduction.  Although zip and gzip 
usually achieve higher ratios, SCZ makes tradeoffs for simplicity, memory
footprint, and runtime speed, - in that order -, while considering diminishing returns. 
For example, gzip compressed a 10-MB file to 1.8-MB, while SCZ compressed it to 2.2-MB.
So gzip saved 8.2-MB, while SCZ saved 7.8-MB. Either way, that's a good portion of
space saved!  Sure, we could go after that last 0.4-MB of extra compression,
but that would more than double the complexity and runtime of SCZ and save only
5% more space.

Although the scz routines are intended for compiling (or linking) into your own 
applications, included are two self-contained application progams, that call
the core copmression routine, and which can be used as stand-alone utilities, or 
as example applications:
    * scz_compress.c - Compresses files.
    * scz_decompress.c - Decompresses files.


Comments:

1. Both scz_compress_lib.c and scz_decompress_lib.c can be included in other
   programs without conflicting each other.  All public (global) variables and
   routine names are prefaced with SCZ_, to minimize conflicts with user code.
   These library files should coexist peacefully with most any other programs.

2. Blocking was added.  Originally, SCZ would compress a whole file or buffer
   in one chunk, no matter how large it was.  This was inefficient for 
   extremely large files, and limited scalability.  The larger the file to be
   compressed, the more ram memory was needed.  Also, SCZ had to fit one
   lookup table to the whole file, even though the redundancy-statistics often
   change throughout large files.  This limited compression quality.
   As well, for streaming applications, it meant no output could be processed
   until all the data came in.
   
   Therefore, an integral 'blocking' capability was added to the SCZ format.
   It enables SCZ routines to process large files or buffers in smaller segments.
   This limits the dynamic memory required to a constant, regardless of the
   file size.  It also provides more efficient compression, because replacement
   symbols can better match the redundancy statistics of smaller portions of
   large files.  And it enables streaming operations - the ability to
   compress/decompress a little bit at a time, continuously, if need be.

   The 'blocking-size' is arbitrary.  It defaults to 4 MB, but can be set
   smaller or larger by changing the value of the "sczbuflen" variable.
   Reducing block size yields greater compression, but consumes more time.

3. An internal checksum was added to SCZ format.  It uses a 8-bit checksum.
   An original checksum is calculated on each input segment and stored as
   one byte in the compressed data.  Upon decompression, a new checksum
   is computed on the decompressed data, and compared to the original
   that was stored.  Any mismatch indicates data corruption.  A match provides
   more than 99.6% confidence that the data is exact and that nothing was lost
   in the compress/storage/decompress process.  (Actually the confidence
   is probably higher than that, because the positions, values, and buffer
   length must to match several other words, and there is checking against magic 
   numerals/boundaries, which would catch many perturbations that would not be
   detected by the checksum.  More checksum bytes would provide even greater
   confidence, but with diminishing returns - adding overhead while only 
   diminishing the remaining less than 0.4% cases.  At least this scheme is 
   better-than-nothing at all.)

4. You are welcome to make variants of the user-access routines, such as:
   Scz_Compress_File, Scz_Compress_Buffer2File, Scz_Compress_Buffer2Buffer,
   or their 'decompress' cousins.  Consider these as examples for how to
   call SCZ's core compress/decompress routines. 

5. SCZ allocates temporary buffer memory in 2-kB chunks as needed.  It repeatedly
   re-uses the memory by maintaining its own free lists.  It uses its own "new"
   and "dispose" routines that work from free-lists, or it gets more memory
   from system when needed.  SCZ routines could free all the buffer memory on
   each return, but since they are often called repeatedily, it is more efficient
   to re-use the buffer between calls.  Many applications complete shortly after
   calling SCZ routines, and the operating system relinquishes the memory on
   exit.  However, if your application continues to run for some time after
   calling your last SCZ routine, you may wish to relinquish all the temporarily
   allocated memory.  Do this be calling "scz_cleanup()" after your last
   scz call.


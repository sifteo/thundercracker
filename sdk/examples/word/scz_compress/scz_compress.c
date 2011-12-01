/*************************************************************************/
/* SCZ_Compress.c - Compress files.  Simple compression.		*/
/*   This file uses the SCZ compression routines to implement a 
     stand-alone file-compression utility.

 ---

 Compile:
   cc -O scz_compress.c -o scz_compress

 Usage:
   scz_compress  file.data		-- Produces "file.scz"
 or:
   scz_compress  file.data  file2	-- Produces "file2"

 Options:
	- b xx  - Use block size xx.  Reducing block size may give
		  greater compression, but take more time.

 SCZ_Compress - LGPL License:
  Copyright (C) 2001, Carl Kindman
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  For updates/info, see:  http://sourceforge.net/projects/scz-compress/

  Carl Kindman 11-21-2004     carlkindman@yahoo.com
		7-5-2005	Added checksums and blocking.
*************************************************************************/

#include "scz_compress_lib.c"




/**********************************************/
/*  Main  - Simple Compression Utility. (SCZ) */
/**********************************************/
int main( int argc, char *argv[] )
{
 char outfname[4096];
 int j, k, verbose=0;

 /* Get the command-line arguments. */
 j = 1;  k = 0;
 while (argc>j)
  { /*argument*/
   if (argv[j][0]=='-')
    { /*optionflag*/
     if (strcasecmp(argv[j],"-v")==0) verbose = 1;
     else
     if (strcasecmp(argv[j],"-b")==0) 
      {
	j++;
 	if (j==argc) {printf("Missing -b blocksize value.\n"); exit(0);}
	if (sscanf(argv[j],"%d",&sczbuflen)!=1) {printf("Bad -b blocksize value '%s'.\n",argv[j]); exit(0);}
      }
     else {printf("\nERROR:  Unknown command line option /%s/.\n", argv[j]); exit(0); }
    } /*optionflag*/
   else
    { /*file*/
     k = k + 1;
     if (strlen(argv[j])>4090) {printf("Error: file name too long.\n"); exit(0);}
     strcpy(outfname,argv[j]);	/* Append .scz suffix. */
     if (outfname[strlen(outfname)-1]=='.') outfname[strlen(outfname)-1] = '\0';
     strcat(outfname, ".scz");
     printf("\n Writing output to file %s\n", outfname);
     Scz_Compress_File( argv[j], outfname );	/* <--- Compress the file. */
    } /*file*/
   j = j + 1;
  } /*argument*/
 if (k==0) { printf("Error: Missing file name. Exiting."); exit(0); }
 
 return 0;
}

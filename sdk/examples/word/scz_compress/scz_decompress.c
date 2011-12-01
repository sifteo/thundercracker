/****************************************************************************/ 
/* SCZ_Decompress.c - Decompresses what SCZ_Compress.c produces.
   This program uses the SCZ decompression routine to implement a 
     stand-alone file-decompression utility.

  Compile:
    cc -O scz_decompress.c -o scz_decompress

  Usage:
    scz_decompress  file.dat.scz		-- Produces "file.dat"
  or:
    scz_decompress  file.dat.scz  file2		-- Produces "file2"

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
                7-5-2005        Added checksums and blocking.
*****************************************************************************/

#include "scz_decompress_lib.c"


/************************************************/
/*  Main  - Simple DeCompression Utility. (SCZ) */
/************************************************/
int main( int argc, char *argv[] )
{
 char fname[4096], *suffix;
 int j=1, k=0, m, verbose=0;

 /* Get the command-line arguments. */
 while (argc>j)
  { /*argument*/
   if (argv[j][0]=='-')
    { /*optionflag*/
     if (strcasecmp(argv[j],"-v")==0) verbose = 1;
     else {printf("\nERROR:  Unknown command line option /%s/.\n", argv[j]); exit(0); }
    } /*optionflag*/
   else
    { /*file*/
     k = k + 1;
     /* Find final '.scz'.  Name must end with .scz.  Remove the .scz suffix. */
     m = strlen(argv[j]);
     if (m>4090) {printf("SCZ Error: file name too long.\n"); exit(0);}
     strcpy(fname,argv[j]);
     suffix = strstr(fname,".scz");
     if (suffix==0) printf("SCZ Warning: Skipping file not ending in '.scz', '%s'\n", argv[j]);
     else
      {
       suffix[0] = '\0';
       if (fname[0]=='\0') printf("SCZ Warning: File '%s' reduces to zero length name, skipping.\n",argv[j]);
       else
        Scz_Decompress_File( argv[j], fname );     /* <--- De-compress the file. */
      }
    } /*file*/
   j = j + 1;
  } /*argument*/
 if (k==0) { printf("Error: Missing file name.\n"); }
 return 0;
}

/*************************************************************************/
/* SCZ_Compress_Lib.c - Compress files or buffers.  Simple compression.	*/
/*
/  This file contains the following user-callable routines:
/    Scz_Compress_File( *infilename, *outfilename );
/    Scz_Compress_Buffer2File( *buffer, N, *outfilename );
/    Scz_Compress_Buffer2Buffer( *inbuffer, N, **outbuffer, *M, lastbuf_flag );
/
/  See below for formal definitions.

  SCZ Method:
   Finds the most frequent pairs, and least frequent chars.
   Uses the least frequent char as 'forcing-char', and other infrequent 
   char(s) as replacement for most frequent pair(s).

   Advantage of working with pairs:  A random access incidence array 
   can be maintained and rapidly searched.
   Repeating the process builds effective triplets, quadruplets, etc..

   At each stage, we are interested only in knowing the least used 
   character(s), and the most common pair(s).

   Process for pairs consumes: 256*256 = 65,536 words or 0.26-MB. 
   (I.E. Really nothing on modern computers.)

   Process could be expanded to triplets, with array of 16.7 words or 
   67 MB.  Recommend going to trees after that.  But have not found
   advantages to going above pairs. On the contrary, pairs are faster
   to search and allow lower granularity replacement (compression).
 ---

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
		9-14-2006       Freeing fixes by David Senterfitt.
*************************************************************************/

#ifndef SCZ_COMPRESSLIB_DEFD
#define SCZ_COMPRESSLIB_DEFD

#include "scz_core.c"
int *scz_freq2=0; 


/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from largest to smaller values.  (A descending list.) */
/*------------------------------------------------------------*/
void scz_add_sorted_nmax( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N )
{
 int j, k=0, m;

 if (value <= list[N-1].value) return;
 while ((k<N) && (list[k].value >= value)) k++;
 if (k==N) return;
 j = N-2;
 while (j>=k)
  { 
   list[j+1].value = list[j].value;  
   for (m=0; m<lngth; m++) list[j+1].phrase[m] = list[j].phrase[m];  
   j--;
  }
 list[k].value = value;
 for (j=0; j<lngth; j++) list[k].phrase[j] = phrase[j];
}


/*------------------------------------------------------------*/
/* Add an item to a value-sorted list of maximum-length N.    */
/* Sort from smaller to larger values.  (An ascending list.)  */
/*------------------------------------------------------------*/
void scz_add_sorted_nmin( struct scz_amalgam *list, unsigned char *phrase, int lngth, int value, int N )
{
 int j, k=0, m;

 while ((k<N) && (list[k].value <= value)) k++;
 if (k==N) return;
 j = N-2;
 while (j>=k) 
  { 
   list[j+1].value = list[j].value;
   for (m=0; m<lngth; m++) list[j+1].phrase[m] = list[j].phrase[m];
   j--;
  }
 list[k].value = value;
 for (j=0; j<lngth; j++) list[k].phrase[j] = phrase[j];
}


/*----------------------------------------------------------------------*/
/* Analyze a buffer to determine the frequency of characters and pairs. */
/*----------------------------------------------------------------------*/
void scz_analyze( struct scz_item *buffer0_hd, int *freq1, int *freq2 )
{
 int j, k;
 struct scz_item *ptr;

 memset( freq1, 0, sizeof(int)*256 );
 memset( freq2, 0, sizeof(int)*256*256 );
 ptr = buffer0_hd;
 if (ptr==0) return;
 k = ptr->ch;
 freq1[k]++;
 ptr = ptr->nxt;
 while (ptr!=0)
  {
   j = k;
   k = ptr->ch;
   freq1[k]++;
   freq2[j*256+k]++;
   ptr = ptr->nxt;
  }
}




/*------------------------------------------------------*/
/* Compress a buffer, step.  Called iteratively.	*/
/*------------------------------------------------------*/
int scz_compress_iter( struct scz_item **buffer0_hd )
{
 int nreplace=250;
 int freq1[256], markerlist[256];
 int i, j, k, replaced, nreplaced, sz3=0, saved=0, saved_pairfreq[256], saved_charfreq[257];
 struct scz_item *head1, *head2, *bufpt, *tmpbuf, *cntpt;
 unsigned char word[10], forcingchar;
 struct scz_amalgam char_freq[257], phrase_freq_max[256];

 /* Examine the buffer. */
 /* Determine histogram of character usage, and pair-frequencies. */
 if (scz_freq2==0) scz_freq2 = (int *)malloc(256*256*sizeof(int));
 scz_analyze( *buffer0_hd, freq1, scz_freq2 );

 /* Initialize rank vectors. */
 memset( saved_pairfreq, 0, 256 * sizeof(int) );
 memset( saved_charfreq, 0, 256 * sizeof(int) );
 for (k=0; k<256; k++)
  {
   char_freq[k].value = 1073741824;
   phrase_freq_max[k].value = 0;
  }

 /* Sort and rank chars. */
 for (j=0; j!=256; j++) 
  {
   word[0] = j;
   scz_add_sorted_nmin( char_freq, word, 1, freq1[j], nreplace+1 );
  }

 /* Sort and rank pairs. */
 i = 0;  
 for (k=0; k!=256; k++) 
  for (j=0; j!=256; j++) 
   if (scz_freq2[j*256+k]!=0)
    {
     word[0] = j;  word[1] = k;
     scz_add_sorted_nmax( phrase_freq_max, word, 2, scz_freq2[j*256+k], nreplace );
    }

 /* Use the least-used character(s) for special expansion symbol(s). I.E. "markers". */
 /* Now go through, and replace any instance of the max_pairs_phrase's with the respective markers. */
 /* And insert before any natural occurrences of the markers, the forcingchar. */
 /*  These two sets should be mutually exclusive, so it should not matter which order this is done. */

 head1 = 0;	head2 = 0;
 forcingchar = char_freq[0].phrase[0];
 k = -1;  j = 0;
 while ((j<nreplace) && (char_freq[j+1].value < phrase_freq_max[j].value - 3))
  { j++;   k = k + phrase_freq_max[j].value - char_freq[j+1].value; }
 nreplaced = j;

 if (nreplaced == 0) return 0; /* Not able to compress this data further with this method. */

 /* Now go through the buffer, and make the following replacements:
    - If equals forcingchar or any of the other maker-chars, then insert forcing char in front of them.
    - If the next pair match any of the frequent_phrases, then replace the phrase by the corresponding marker.
 */


/* Define a new array, phrase_freq_max2[256], which is a lookup array for the first character of
   a two-character phrase.  Each element points to a list of second characters with "j" indices.
   First chars which have none are null, stopping the search immediately.
*/
memset( sczphrasefreq, 0, 256 * sizeof(struct scz_item2 *) );
for (j=0; j!=256; j++) markerlist[j] = nreplaced+1;
for (j=0; j!=nreplaced+1; j++) markerlist[ char_freq[j].phrase[0] ] = j;
for (j=0; j!=nreplaced; j++)
 if (phrase_freq_max[j].value>0)
  {
   scztmpphrasefreq = new_scz_item2();
   scztmpphrasefreq->ch = phrase_freq_max[j].phrase[1];
   scztmpphrasefreq->j = j;
   k = phrase_freq_max[j].phrase[0];
   scztmpphrasefreq->nxt = sczphrasefreq[k];
   sczphrasefreq[k] = scztmpphrasefreq;
  }


 /* First do a tentative check. */
 bufpt = *buffer0_hd;
 if (nreplaced>0)
 while (bufpt!=0)
  {
   replaced = 0;
   if (bufpt->nxt!=0)
    {  /* Check for match to frequent phrases. */
     k = bufpt->ch;
     if (sczphrasefreq[k]==0) j = nreplaced;
     else
      { unsigned char tmpch;
	tmpch = bufpt->nxt->ch; 
	scztmpphrasefreq = sczphrasefreq[k];
	while ((scztmpphrasefreq!=0) && (scztmpphrasefreq->ch != tmpch)) scztmpphrasefreq = scztmpphrasefreq->nxt;
        if (scztmpphrasefreq==0) j = nreplaced; else j = scztmpphrasefreq->j;
      }

     if (j<nreplaced)
      { /* If match, the phrase would be replaced with corresponding marker. */
	saved++;
	saved_pairfreq[j]++;		/* Keep track of how many times this phrase occured. */
	bufpt = bufpt->nxt->nxt;	/* Skip over. */
	replaced = 1;
      }
    }
   if (!replaced)
    {  /* Check for match of marker characters. */
     j = markerlist[ bufpt->ch ];
     if (j<=nreplaced)
      {	/* If match, insert forcing character. */
	saved--;
	saved_charfreq[j]--;		/* Keep track of how many 'collisions' this marker-char caused. */
      }
     bufpt = bufpt->nxt;
    }
  }

 // printf("%d saved\n", saved);
 if (saved<=1) return 0; /* Not able to compress this data further with this method. Buffer unchanged. */

 /* Now we know which marker/phrase combinations do not actually pay. */
 /* Reformulate the marker list with reduced set. */
 /* The least frequent chars become the forcing char and the marker characters. */
 /* Store out the forcing-char, markers and replacement phrases after a magic number. */
 scz_add_item( &head1, &head2, char_freq[0].phrase[0] );	/* First add forcing-marker (escape-like) value. */
 scz_add_item( &head1, &head2, 0 );	/* Next, leave place-holder for header-symbol-count. */
 cntpt = head2;
 k = 0;  saved = 0;
 for (j=0; j<nreplaced; j++)
  if (saved_pairfreq[j] + saved_charfreq[j+1] > 3)
   { unsigned char ch;
    saved = saved + saved_pairfreq[j] + saved_charfreq[j+1] - 3;
    ch = char_freq[j+1].phrase[0];
    scz_add_item( &head1, &head2, ch );  /* Add phrase-marker. */
    char_freq[k+1].phrase[0] = ch;

    ch = phrase_freq_max[j].phrase[0];
    scz_add_item( &head1, &head2, ch );	 /* Add phrase 1st char. */
    phrase_freq_max[k].phrase[0] = ch;

    ch = phrase_freq_max[j].phrase[1];
    scz_add_item( &head1, &head2, ch );	 /* Add phrase 2nd char. */
    phrase_freq_max[k].phrase[1] = ch;
    k++;
   }
 saved = saved + saved_charfreq[0];
 if ((k == 0) || (saved < 6)) 
  { /* Not able to compress this data further with this method. Leave buffer basically unchanged. */
   /* Free the old lists. */
   for (j=0; j!=256; j++) 
    while (sczphrasefreq[j] != 0)
     { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }

   /* Free the temporarily added items.        */   /* ****   DWS, 05Sep2006, Added to fix memory leak.       */
   while (head1 != 0)
    {
     bufpt = head1->nxt;
     sczfree(head1);
     head1 = bufpt;
    }

   return 0; 
  }

 sz3 = 3 * (k + 1);
 cntpt->ch = k;		/* Place the header-symbol-count. */
 nreplaced = k;
 scz_add_item( &head1, &head2, 91 );	/* Magic barrier. */

 /* Update the phrase_freq tree. */
 /* First free the old list. */
 for (j=0; j!=256; j++) 
  while (sczphrasefreq[j] != 0)
   { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }
 /* Now make the new list. */
 for (j=0; j!=nreplaced; j++)
  if (phrase_freq_max[j].value>0)
   {
    scztmpphrasefreq = new_scz_item2();
    scztmpphrasefreq->ch = phrase_freq_max[j].phrase[1];
    scztmpphrasefreq->j = j;
    k = phrase_freq_max[j].phrase[0];
    scztmpphrasefreq->nxt = sczphrasefreq[k];
    sczphrasefreq[k] = scztmpphrasefreq;
   }

for (j=0; j!=256; j++) markerlist[j] = nreplaced+1;
for (j=0; j!=nreplaced+1; j++) markerlist[ char_freq[j].phrase[0] ] = j;
// printf("Replacing %d\n", nreplaced);

 bufpt = *buffer0_hd;
 while (bufpt!=0)
  {
   sz3++;
   replaced = 0;
   if (bufpt->nxt!=0)
    {  /* Check for match to frequent phrases. */
     k = bufpt->ch;
     if (sczphrasefreq[k]==0) j = nreplaced;
     else
      { unsigned char tmpch;
	tmpch = bufpt->nxt->ch; 
	scztmpphrasefreq = sczphrasefreq[k];
	while ((scztmpphrasefreq!=0) && (scztmpphrasefreq->ch != tmpch)) scztmpphrasefreq = scztmpphrasefreq->nxt;
        if (scztmpphrasefreq==0) j = nreplaced; else j = scztmpphrasefreq->j;
      }

     if (j<nreplaced)
      { /* If match, replace phrase with corresponding marker. */
	bufpt->ch = char_freq[j+1].phrase[0];
	tmpbuf = bufpt->nxt;
	bufpt->nxt = tmpbuf->nxt;
	sczfree( tmpbuf );
	replaced = 1;
      }
    }
   if (!replaced)
    {  /* Check for match of marker characters. */
     j = markerlist[ bufpt->ch ];
     if (j<=nreplaced)
      {	/* If match, insert forcing character. */
       tmpbuf = new_scz_item();
       tmpbuf->ch = bufpt->ch;
       tmpbuf->nxt = bufpt->nxt;
       bufpt->nxt = tmpbuf;
       bufpt->ch = forcingchar;
       bufpt = tmpbuf;
       sz3 = sz3 + 1;
      }
    }

   bufpt = bufpt->nxt;
  }

 /* Now attach header to main buffer. */
 head2->nxt = *buffer0_hd;
 *buffer0_hd = head1;

 /* Free the old list. */
 for (j=0; j!=256; j++) 
  while (sczphrasefreq[j] != 0)
   { scztmpphrasefreq = sczphrasefreq[j]; sczphrasefreq[j] = sczphrasefreq[j]->nxt; sczfree2(scztmpphrasefreq); }

 return sz3;
}





/*******************************************************************/
/* Scz_Compress - Compress a buffer.  Entry-point to Scz_Compress. */
/*  Compresses the buffer passed in.				   */
/* Pass-in the buffer and its initial size, as sz1. 		   */
/* Returns 1 on success, 0 on failure.				   */
/*******************************************************************/
int Scz_Compress_Seg( struct scz_item **buffer0_hd, int sz1 )
{
 struct scz_item *tmpbuf_hd=0, *tmpbuf_tl=0;
 int sz2, iter=0;

 /* Compress. */
 sz2 = scz_compress_iter( buffer0_hd );
 while ((sz2!=0) && (iter<255))
  {
   sz1 = sz2;
   sz2 = scz_compress_iter( buffer0_hd );
   iter++;  
  }

 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, 101 );   /* Place magic start-number(s). */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, 98 );
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, iter );  /* Place compression count. */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, sz1>>16 );         /* Place size count (MSB). */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, (sz1>>8) & 255 );  /* Place size count. */
 scz_add_item( &tmpbuf_hd, &tmpbuf_tl, sz1 & 255 );       /* Place size count (LSB). */

 tmpbuf_tl->nxt = *buffer0_hd;
 *buffer0_hd = tmpbuf_hd;
 return 1;
}



long Scz_get_file_length( FILE *fp )
{
  long len, cur;
  cur = ftell( fp );            /* remember where we are */
  fseek( fp, 0L, SEEK_END );    /* move to the end */
  len = ftell( fp );            /* get position there */
  fseek( fp, cur, SEEK_SET );   /* return back to original position */
  return( len );
}




/************************************************************************/
/* Scz_Compress_File - Compresses input file to output file.		*/
/*  First argument is input file name.  Second argument is output file	*/
/*  name.  The two file names must be different.			*/
/*									*/
/************************************************************************/
int Scz_Compress_File( char *infilename, char *outfilename )
{
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, flen, buflen;
 unsigned char ch, chksum;
 FILE *infile=0, *outfile=0;

 infile = fopen(infilename,"rb");
 if (infile==0) {printf("ERROR: Cannot open input file '%s'.  Exiting\n", infilename); exit(1);}

 outfile = fopen(outfilename,"wb");
 if (outfile==0) {printf("ERROR: Cannot open output file '%s' for writing.  Exiting\n", outfilename); exit(1);}

 flen = Scz_get_file_length( infile );
 buflen = flen / sczbuflen + 1;
 buflen = flen / buflen + 1;
 if (buflen>=SCZ_MAX_BUF) {printf("Error: Buffer length too large.\n"); exit(0);}

 /* Read file into linked list. */
 ch = getc(infile);
 while (!feof(infile))
  { /*outerloop*/

    chksum = 0;  szi = 0;
    buffer0_hd = 0;   buffer0_tl = 0;
    while ((!feof(infile)) && (szi < buflen))
     {
      scz_add_item( &buffer0_hd, &buffer0_tl, ch );
      sz1++;  szi++;  chksum += ch;  
      ch = getc(infile);
     }

   success = success & Scz_Compress_Seg( &buffer0_hd, szi );

   /* Write the file out. */
   while (buffer0_hd!=0)
    {
     fputc( buffer0_hd->ch, outfile );
     sz2++;
     bufpt = buffer0_hd;
     buffer0_hd = buffer0_hd->nxt;
     sczfree(bufpt);
    }
   fprintf(outfile,"%c", chksum); 
   sz2++;
   if (feof(infile)) fprintf(outfile,"]");	/* Place no-continuation marker. */
   else fprintf(outfile,"[");			/* Place continuation marker. */
   sz2++;

  } /*outerloop*/
 fclose(infile);
 fclose(outfile);

 printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
 printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
 free(scz_freq2);
 scz_freq2 = 0;
 return success;
}




/************************************************************************/
/* Scz_Compress_Buffer2File - Compresses character array input buffer	*/
/*  to an output file.  This routine is handy for applications wishing	*/
/*  to compress their output directly while saving to disk.		*/
/*  First argument is input array, second is the array's length, and	*/
/*  third is the output file name to store to.				*/
/*  (It also shows how to compress and write data out in segments for	*/
/*   very large data sets.)						*/
/*									*/
/************************************************************************/
int Scz_Compress_Buffer2File( unsigned char *buffer, int N, char *outfilename )
{
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, buflen;
 unsigned char chksum;
 FILE *outfile=0;

 outfile = fopen(outfilename,"wb");
 if (outfile==0) {printf("ERROR: Cannot open output file '%s' for writing.  Exiting\n", outfilename); exit(1);}

 buflen = N / sczbuflen + 1;
 buflen = N / buflen + 1;
 if (buflen>=SCZ_MAX_BUF) {printf("Error: Buffer length too large.\n"); exit(0);}

 while (sz1 < N)
  { /*outerloop*/

    chksum = 0;  szi = 0;
    if (N-sz1 < buflen) buflen = N-sz1;

    /* Convert buffer into linked list. */
    buffer0_hd = 0;   buffer0_tl = 0;
    while (szi < buflen)
     {
      scz_add_item( &buffer0_hd, &buffer0_tl, buffer[sz1] );
      chksum += buffer[sz1];  
      sz1++;  szi++;  
     }

   success = success & Scz_Compress_Seg( &buffer0_hd, szi );

   /* Write the file out. */
   while (buffer0_hd!=0)
    {
     fputc( buffer0_hd->ch, outfile );
     sz2++;
     bufpt = buffer0_hd;
     buffer0_hd = buffer0_hd->nxt;
     sczfree(bufpt);
    }
   fprintf(outfile,"%c", chksum); 
   sz2++;
   if (sz1 >= N) fprintf(outfile,"]");	/* Place no-continuation marker. */
   else fprintf(outfile,"[");		/* Place continuation marker. */
   sz2++;

  } /*outerloop*/
 fclose(outfile);

 printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
 printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
 free(scz_freq2);
 scz_freq2 = 0;
 return success;
}



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
/*									*/
/************************************************************************/
int Scz_Compress_Buffer2Buffer( char *inbuffer, int N, char **outbuffer, int *M, int lastbuf_flag )
{
 struct scz_item *buffer0_hd=0, *buffer0_tl=0, *bufpt;
 int sz1=0, sz2=0, szi, success=1, buflen;
 unsigned char chksum;

 buflen = N;
 if (buflen>=SCZ_MAX_BUF) {printf("Error: Buffer length too large.\n"); exit(0);}

 chksum = 0;  szi = 0;
 if (N-szi < buflen) buflen = N-szi;
 /* Convert buffer into linked list. */
 buffer0_hd = 0;   buffer0_tl = 0;
 while (szi < buflen)
  {
   scz_add_item( &buffer0_hd, &buffer0_tl, inbuffer[sz1] );
   chksum += inbuffer[sz1];  
   sz1++;  szi++;
  }

 success = success & Scz_Compress_Seg( &buffer0_hd, szi );

 /* Convert list into buffer. */
 sz2 = 0;	/* Get buffer size. */
 bufpt = buffer0_hd;
 while (bufpt!=0) { sz2++;  bufpt = bufpt->nxt;  }
 *outbuffer = (char *)malloc((sz2+2)*sizeof(char));
 sz2 = 0;
 while (buffer0_hd!=0)
  {
   (*outbuffer)[sz2++] = buffer0_hd->ch;
   bufpt = buffer0_hd;
   buffer0_hd = buffer0_hd->nxt;
   sczfree(bufpt);
  }
 (*outbuffer)[sz2++] = chksum; 
 if (lastbuf_flag) (*outbuffer)[sz2++] = ']';	/* Place no-continuation marker. */
 else (*outbuffer)[sz2++] = '[';			/* Place continuation marker. */

 *M = sz2;

 // printf("Initial size = %d,  Final size = %d\n", sz1, sz2);
 // printf("Compression ratio = %g : 1\n", (float)sz1 / (float)sz2 );
 free(scz_freq2);
 scz_freq2 = 0;
 return success;
}

#endif /* SCZ_COMPRESSLIB_DEFD */

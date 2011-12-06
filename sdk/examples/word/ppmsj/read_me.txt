        1. Short DESCRIPTION.
    PPMs is a small clone of PPMd compressor [1], it uses the same PPMII
compression algorithm [2-3]. PPMs is intended for embedding in programs that
have some restrictions on the used memory, so it can be configured for
memory requrements from 0.06MB to 1MB. PPMs can be recompiled as a library
and PPMd.h header file will be the interface to this library. Model order
selection option (-o) choose balance between execution speed and compression
performance.

        2. Distribution CONTENTS.
    read_me.txt  - this file;
    PPMd.h, PPMdType.h - header files;
    Coder.hpp, SubAlloc.hpp, Model.cpp, PPMs.cpp - code sources;
    makefile.imk - makefile for IntelC v.4.0;
    makefile.mak - makefile for BorlandC v.5.01;
    PPMs.exe     - compressor with 1MB memory heap (CC rate - 2.15bpb);

        3. LEGAL stuff.
    You can not misattribute authorship on algorithm or code sources,  You can
not patent algorithm or its parts, all other things are allowed and welcomed.
    Dmitry Subbotin  and me  have authorship  rights on  code sources.  Dmitry
Subbotin owns authorship rights on  his variation of rangecoder algorithm  and
I own authorship rights  on my variation of  PPM algorithm, this variation  is
named  PPMII  (PPM  with  Information  Inheritance).  Our  authorship  must be
mentioned in documentation on program that uses these algorithms.

        4. DIFFERENCES between variants.
        Jul 19, 2004  var.I
    Initial release;
        Sep 23, 2004  var.I rev.1
    Size of memory heap can be varied now from 64KB to 1MB, memory
requirements are (_MEM_CONFIG*(1+1/16)+6) KB;
        Feb 21, 2006  var.J
    This variant is produced only for synchronization with main PPMd program;

        5. REFERENCES.
    [1] D.Shkarin 'PPMd - fast PPMII compressor for textual data'
http://www.compression.ru/ds/ppmdi1.rar
    [2] D.Shkarin 'Improving the efficiency of PPM algorithm', in Russian,
http://www.compression.ru/download/articles/ppm/shkarin_ppi2001_pdf.rar
    [3] D.Shkarin 'PPM: one step to practicality', in English,
http://www.compression.ru/download/articles/ppm/shkarin_2002dcc_ppmii_pdf.rar

    AUTHOR SHALL NOT BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL,  INCIDENTAL,
OR CONSEQUENTIAL  DAMAGES ARISING  OUT OF  ANY USE  OF THIS  SOFTWARE. YOU USE
THIS PROGRAM AT YOUR OWN RISK.

                                        Dancy compression!
                                        Dmitry Shkarin
                                        E-mail: dmitry.shkarin@mtu-net.ru

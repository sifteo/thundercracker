include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include"
                    "${CMAKE_CURRENT_BINARY_DIR}/../include")

set(BWT_SRC bwt/bwt.c bwt/sais.c)
set(UNBWT_SRC unbwt/LL.c unbwt/LRI.c unbwt/LRI8v1.c
              unbwt/LRI8v2.c unbwt/basisLF.c unbwt/basisPSI.c
              unbwt/biPSIv1.c unbwt/biPSIv2.c
              unbwt/indexLFv1.c unbwt/indexLFv2.c
              unbwt/indexPSIv1.c unbwt/indexPSIv2.c
              unbwt/unlimTLF.c unbwt/unlimTPSI.c
              unbwt/bw94.c unbwt/mergedTL.c unbwt/mergedTPSI.c
              unbwt/saLF.c unbwt/saPSI.c)
set(BWTS_SRC bwts/bwts.c)
set(SST_SRC sst/Bx.c sst/distancecoder.c sst/frequencycount.c
            sst/ifc.c sst/inversioncoder.c sst/mtf.c sst/sdcoder.c
            sst/sortbyrank.c sst/srcoder.c sst/timestamp0.c
            sst/transpose.c sst/wfc.c)

## libopenbwt ##
add_library(openbwt obwt_version.c obwt_ISAs.c ${BWT_SRC} ${UNBWT_SRC} ${BWTS_SRC} ${SST_SRC})
install(TARGETS openbwt
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
set_target_properties(openbwt PROPERTIES
  VERSION   "${LIBRARY_VERSION_FULL}"
  SOVERSION "${LIBRARY_VERSION_MAJOR}"
  DEFINE_SYMBOL OBWT_BUILD_DLL
  CLEAN_DIRECT_OUTPUT 1)
if(OPENMP_FOUND AND CMAKE_COMPILER_IS_GNUCC AND BUILD_SHARED_LIBS)
  target_link_libraries(openbwt gomp)
endif(OPENMP_FOUND AND CMAKE_COMPILER_IS_GNUCC AND BUILD_SHARED_LIBS)

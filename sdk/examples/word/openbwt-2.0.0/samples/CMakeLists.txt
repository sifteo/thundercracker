## Targets ##
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include"
                    "${CMAKE_CURRENT_BINARY_DIR}/../include")
link_directories("${CMAKE_CURRENT_BINARY_DIR}/../src")

foreach(src bwt_old unbwt_old sst_test bwt unbwt bwts unbwts)
  add_executable(${src} ${src}.c)
  target_link_libraries(${src} openbwt)
  if(OPENMP_FOUND AND CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(${src} gomp)
  endif(OPENMP_FOUND AND CMAKE_COMPILER_IS_GNUCC)
endforeach(src)

if(UNIX)
  find_library(MATH_LIB m)
  if(MATH_LIB)
    target_link_libraries(sst_test ${MATH_LIB})
  endif(MATH_LIB)
endif(UNIX)

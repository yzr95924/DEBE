#src/util

aux_source_directory(. UTIL_SRC)

add_library(UtilCore ${UTIL_SRC})

target_link_libraries(UtilCore ${THRID_OBJ} pthread)
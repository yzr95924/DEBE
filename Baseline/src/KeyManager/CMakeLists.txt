#src/util

aux_source_directory(. KM_SRC)

add_library(KMCore ${KM_SRC})

target_link_libraries(KMCore ${THRID_OBJ} pthread)
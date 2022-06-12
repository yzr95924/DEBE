aux_source_directory(. INDEX_SRC)

add_library(IndexCore ${INDEX_SRC})

target_link_libraries(IndexCore ${THIRD_OBJ} ServerCore UtilCore) 
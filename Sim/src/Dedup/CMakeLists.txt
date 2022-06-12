aux_source_directory(. DEDUP_SRC)
add_library(DedupCore ${DEDUP_SRC})
target_link_libraries(DedupCore ${UTIL_OBJ} ${CRYPTO_OBJ})
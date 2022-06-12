aux_source_directory(. CLIENT_SRC)
add_library(ClientCore ${CLIENT_SRC})
target_link_libraries(ClientCore CommCore IASCore)
aux_source_directory(. SERVER_SRC)
add_library(ServerCore ${SERVER_SRC})
target_link_libraries(ServerCore IndexCore)
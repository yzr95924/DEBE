#src/app
add_executable(DAEServer daeServer.cc)
target_link_libraries(DAEServer ${FINAL_OBJ})

add_executable(DAEClient daeClient.cc)
target_link_libraries(DAEClient ${FINAL_OBJ})

add_executable(KeyManager keyManager.cc)
target_link_libraries(KeyManager ${FINAL_OBJ})
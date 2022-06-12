#src/app
set(SGXOPENSSL_INCLUDE_PATH /opt/intel/sgxssl/include)
set(SGXOPENSSL_LIBRARY_PATH /opt/intel/sgxssl/lib64)
include_directories(${SGXOPENSSL_INCLUDE_PATH})
link_directories(${SGXOPENSSL_LIBRARY_PATH})

add_executable(DEBEServer dbeServer.cc)
target_link_libraries(DEBEServer ${FINAL_OBJ})

add_executable(DEBEClient dbeClient.cc)
target_link_libraries(DEBEClient ${FINAL_OBJ})

add_executable(compressSYN synCompress.cc)
target_link_libraries(compressSYN lz4 UtilCore ${OPENSSL_LIBRARY_OBJ})
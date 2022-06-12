# src/storeEnclave
# for ocall src
aux_source_directory(ocallSrc/ OCALL_SRC)
# for ecall src
aux_source_directory(ecallSrc/ecall ECALL_SRC)
aux_source_directory(ecallSrc/ecallIndex ECALL_INDEX_SRC)
aux_source_directory(ecallSrc/ecallStore ECALL_STORE_SRC)
aux_source_directory(ecallSrc/ecallUtil ECALL_UTIL_SRC)

aux_source_directory(ecallSrc/moti ECALL_MOTI_SRC)

set(TRUST_SRC ${ECALL_SRC} ${ECALL_INDEX_SRC} ${ECALL_STORE_SRC} ${ECALL_UTIL_SRC} ${ECALL_MOTI_SRC})
set(UNTRUST_SRC ${OCALL_SRC})

set(SGXOPENSSL_INCLUDE_PATH /opt/intel/sgxssl/include)
set(SGXOPENSSL_LIBRARY_PATH /opt/intel/sgxssl/lib64)
set(ENCLAVE_INCLUDE include)

set(EDL_SEARCH_PATHS ${SGXOPENSSL_INCLUDE_PATH} /opt/intel/sgxsdk/include)
set(LDS storeEnclave.lds)

set(SGXOPENSSL_UNTRUST_OBJECTS sgx_usgxssl)

set(SGXSSL_Library_Name sgx_tsgxssl)
set(OpenSSL_Crypto_Library_Name sgx_tsgxssl_crypto)


include_directories(${SGXOPENSSL_INCLUDE_PATH} ${ENCLAVE_INCLUDE})
link_directories(${SGXOPENSSL_LIBRARY_PATH})

add_enclave_library(storeEnclave
                    USE_SGXSSL
                    SRCS ${TRUST_SRC}
                    EDL storeEnclave.edl
                    EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS}
                    LDSCRIPT ${LDS})

enclave_sign(storeEnclave KEY storeEnclave.pem CONFIG storeEnclave.config.xml)

add_untrusted_library(EnclaveCore
                    STATIC
                    SRCS ${UNTRUST_SRC}
                    EDL storeEnclave.edl
                    EDL_SEARCH_PATHS ${EDL_SEARCH_PATHS})

target_link_libraries(EnclaveCore ${SGX_UNTRUST_MUST_LINK} ${SGXOPENSSL_UNTRUST_OBJECTS})
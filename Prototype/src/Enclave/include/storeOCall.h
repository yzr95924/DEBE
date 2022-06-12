#ifndef STORE_OCALL_H 
#define STORE_OCALL_H 

#include "ocallUtil.h"
#include "../../../include/storageCore.h"
#include "../../../include/clientVar.h"
#include "../../../include/absDatabase.h"
#include "../../../include/dataWriter.h"
#include "../../../include/enclaveRecvDecoder.h"

#include "sgx_urts.h"

// forward declaration
class EnclaveRecvDecoder;

/**
 * @brief define the variable use in Ocall
 * 
 */
namespace OutEnclave {
    // ocall for upload
    extern StorageCore* storageCoreObj_;
    extern DataWriter* dataWriterObj_;
    extern AbsDatabase* indexStoreObj_;

    // ocall for restore
    extern EnclaveRecvDecoder* enclaveRecvDecoderObj_;
    extern string myName_;

    // for persistence
    extern ofstream outSealedFile_;
    extern ifstream inSealedFile_;

    // rw lock for index
    extern pthread_rwlock_t outIdxLck_;
    
    /**
     * @brief setup the ocall var
     * 
     * @param dataWriterObj the pointer to the data writer
     * @param indexStoreObj the pointer to the index
     * @param storageCoreObj the pointer to the storageCoreObj
     * @param enclaveDecoderObj the pointer to the enclave recvDecoder
     */
    void Init(DataWriter* dataWriterObj, AbsDatabase* indexStoreObj,
        StorageCore* storageCoreObj, EnclaveRecvDecoder* enclaveRecvDecoderObj);

    /**
     * @brief destroy the ocall var
     * 
     */
    void Destroy();
};

/**
 * @brief exit the enclave with error message
 * 
 * @param error_msg 
 */
void Ocall_SGX_Exit_Error(const char* error_msg);

/**
 * @brief dump the inside container to the outside buffer
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_WriteContainer(void* outClient);

/**
 * @brief printf interface for Ocall
 * 
 * @param str input string 
 */
void Ocall_Printf(const char* str);

/**
 * @brief persist the buffer to file 
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_UpdateFileRecipe(void* outClient);

/**
 * @brief update the outside index store 
 * 
 * @param ret return result
 * @param key pointer to the key
 * @param keySize the key size
 * @param buffer pointer to the buffer
 * @param bufferSize the buffer size
 */
void Ocall_UpdateIndexStoreBuffer(bool* ret, const char* key, size_t keySize,
    const uint8_t* buffer, size_t bufferSize);

/**
 * @brief read the outside index store
 *
 * @param ret return result 
 * @param key pointer to the key
 * @param keySize the key size
 * @param retVal pointer to the buffer <return>
 * @param expectedRetValSize the expected buffer size <return>
 * @param outClient the out-enclave client ptr
 */
void Ocall_ReadIndexStore(bool* ret, const char* key, size_t keySize,
    uint8_t** retVal, size_t* expectedRetValSize, void* outClient);

/**
 * @brief write the data to the disk file
 * 
 * @param sealedFileName the name of the sealed file
 * @param sealedDataBuffer sealed data buffer
 * @param sealedDataSize sealed data size
 */
void Ocall_WriteSealedData(const char* sealedFileName, uint8_t* sealedDataBuffer, size_t sealedDataSize);

/**
 * @brief init the file output stream
 *
 * @param ret the return result 
 * @param sealedFileName the sealed file name
 */
void Ocall_InitWriteSealedFile(bool* ret, const char* sealedFileName);

/**
 * @brief close the file output stream
 * 
 * @param sealedFileName the sealed file name
 */
void Ocall_CloseWriteSealedFile(const char* sealedFileName);


/**
 * @brief read the sealed data from the file
 * 
 * @param sealedFileName the sealed file
 * @param dataBuffer the data buffer
 * @param sealedDataSize the size of sealed data
 */
void Ocall_ReadSealedData(const char* sealedFileName, uint8_t* dataBuffer, uint32_t sealedDataSize);

/**
 * @brief get current time from the outside
 * 
 */
void Ocall_GetCurrentTime(uint64_t* retTime);

/**
 * @brief Init the unseal file stream 
 *
 * @param fileSize the return file size
 * @param sealedFileName the sealed file name
 */
void Ocall_InitReadSealedFile(size_t* fileSize, const char* sealedFileName);
/**
 * @brief close the file input stream
 * 
 * @param sealedFileName the sealed file name
 */
void Ocall_CloseReadSealedFile(const char* sealedFileName);

/**
 * @brief Print the content of the buffer
 * 
 * @param buffer the input buffer
 * @param len the length in byte
 */
void Ocall_PrintfBinary(const uint8_t* buffer, size_t len);

/**
 * @brief Get the required container from the outside application
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_GetReqContainers(void* outClient);

/**
 * @brief send the restore chunks to the client
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_SendRestoreData(void* outClient);

/**
 * @brief query the outside deduplication index 
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_QueryOutIndex(void* outClient);

/**
 * @brief update the outside deduplication index
 * 
 * @param outClient the out-enclave client ptr
 */
void Ocall_UpdateOutIndex(void* outClient);

/**
 * @brief generate the UUID
 * 
 * @param id the uuid buffer
 * @param len the id len
 */
void Ocall_CreateUUID(uint8_t* id, size_t len);

#endif //  ENC_OCALL_H!
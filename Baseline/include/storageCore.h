#ifndef BASICDEDUP_STORAGECORE_h
#define BASICDEDUP_STORAGECORE_h

#include "configure.h"
#include "clientVar.h"
#include "storeOCall.h"

using namespace std;

class StorageCore {
    private:
        string myName_ = "StorageCore";

        // written data size
        uint64_t writtenDataSize_ = 0;
        uint64_t writtenChunkNum_ = 0;

        /**
         * @brief write the data to a container according to the given metadata
         * 
         * @param key chunk metadata
         * @param data content
         * @param curContainer the pointer to the current container
         * @param curClient the ptr to the current client
         */
        void WriteContainer(RecipeEntry_t* key, char* data,
            Container_t* curContainer, ClientVar* curClient);

    public:

        /**
         * @brief Construct a new Storage Core object
         * 
         */
        StorageCore();
        
        /**
         * @brief Destroy the Storage Core object
         * 
         */
        ~StorageCore();

        /**
         * @brief save the chunk to the storage server
         * 
         * @param chunkData the chunk data buffer
         * @param chunkSize the chunk size
         * @param chunkAddr the chunk address (return)
         * @param curClient the prt to current client
         */
        void SaveChunk(char* chunkData, uint32_t chunkSize, RecipeEntry_t* chunkAddr, 
            ClientVar* curClient);

        /**
         * @brief update the file recipe to the disk
         * 
         * @param recipeBuffer the pointer to the recipe buffer
         * @param recipeEntryNum the number of recipe entries
         * @param fileRecipeHandler the recipe file handler
         */
        void UpdateRecipeToFile(const uint8_t* recipeBuffer, size_t recipeEntryNum, 
            ofstream& fileRecipeHandler);
        
        /**
         * @brief finalize the file recipe
         * 
         * @param recipeHead the recipe header
         * @param fileRecipeHandler the recipe file handler
         */
        void FinalizeRecipe(FileRecipeHead_t* recipeHead, 
            ofstream& fileRecipeHandler);
};


#endif // !BASICDEDUP_STORAGECORE_h

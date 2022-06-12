#ifndef BASICDEDUP_STORAGECORE_h
#define BASICDEDUP_STORAGECORE_h

#include "configure.h"
#include "chunkStructure.h"
#include "messageQueue.h"
#include "absDatabase.h"
#include "dataWriter.h"
#include "define.h"

#define NEW_FILE_NAME_HASH 1
#define OLD_FILE_NAME_HASH 2

using namespace std;

class StorageCore {
    private:
        string myName_ = "StorageCore";
        std::string recipeNamePrefix_;
        std::string recipeNameTail_;
    public:
        /**
         * @brief finalize the file recipe
         * 
         * @param recipeHead the recipe header
         * @param fileRecipeHandler the recipe file handler
         */
        void FinalizeRecipe(FileRecipeHead_t* recipeHead, 
            ofstream& fileRecipeHandler);

        /**
         * @brief update the file recipe to the disk
         * 
         * @param recipeBuffer the pointer to the recipe buffer
         * @param recipeEntryNum the number of recipe entries
         * @param fileRecipeHandler the recipe file handler
         */
        void UpdateRecipeToFile(const uint8_t* recipeBuffer, size_t recipeEntryNum, ofstream& fileRecipeHandler);

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
};


#endif // !BASICDEDUP_STORAGECORE_h

/**
 * @file restoreWriter.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief restore the data to the disk
 * @version 0.1
 * @date 2020-01-02
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#ifndef BASICDEDUP_RESTORE_WRITER_h
#define BASICDEDUP_RESTORE_WRITER_h

#include "configure.h"
#include "chunkStructure.h"
#include "messageQueue.h"

using namespace std;
extern Configure config;

class RestoreWriter {
    private:
        string myName_ = "RestoreWriter";

        std::ofstream restoreFile_;
        FILE* outputFile_;

        uint64_t totalRecvNum_ = 0;
        uint64_t totalWrittenSize_ = 0;

        double totalTime_ = 0;
        struct timeval sTotalTime_;
        struct timeval eTotalTime_;

        MessageQueue<Chunk_t>* inputMQ_;

#if (RESTORE_WRITER_BREAKDOWN == 1)
        double restoreWriteTime_ = 0;
        struct timeval sRestoreTime_;
        struct timeval eRestoreTime_;
#endif

    public:

        /**
         * @brief Construct a new Restore Writer object
         * 
         * @param fileName restore file name
         * @param recvDecodeObj 
         */
        RestoreWriter(string fileName);
        
        /**
         * @brief Destroy the Restore Writer object
         * 
         */
        ~RestoreWriter();

        /**
         * @brief the main process
         * 
         */
        void Run();

        /**
         * @brief Set the InputMQ object
         * 
         * @param inputMQ the inputMQ
         */
        void SetInputMQ(MessageQueue<Chunk_t>* inputMQ) {
            inputMQ_ = inputMQ;
            return ;
        }
};


#endif // !BASICDEDUP_RESTORE_WRITER_h
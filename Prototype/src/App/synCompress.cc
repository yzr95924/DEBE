/**
 * @file compressionTest.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief tset the compression 
 * @version 0.1
 * @date 2021-07-14
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <lz4.h>

#include "../../include/define.h"
#include "../../include/chunkStructure.h"
#include "../../include/cryptoPrimitive.h"
#include "../../include/compressGen.h"

using namespace std;
// use "using" as the "typedef"
struct timeval sTime;
struct timeval eTime;
double compressTime_ = 0;

void Usage() {
    fprintf(stderr, "./compressSYN -i [expected trace size (MiB)] -o [output trace file] -s [seed]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    CryptoPrimitive* cryptoTool;
    cryptoTool = new CryptoPrimitive(AES_256_GCM, SHA_256);
    EVP_MD_CTX* mdCtx = EVP_MD_CTX_new();

    // ------ main process ------
    const char optString[] = "i:o:s:";
    int option;
    if (argc < sizeof(optString)) {
        fprintf(stderr, "wrong argc: %d\n", argc);
        Usage();
    }
    int seed = 0;
    string inputFileName;
    string outputFileName;
    size_t totalProcessSizeMiB = 0;
    while ((option = getopt(argc, argv, optString)) != -1) {
        switch (option) {
            case 'i': {
                totalProcessSizeMiB = atol(optarg);
                fprintf(stderr, "expected trace size (MiB): %lu\n", totalProcessSizeMiB);
                break;
            }
            case 'o': {
                outputFileName.assign(optarg);
                fprintf(stderr, "output trace file: %s\n", outputFileName.c_str());
                break;
            }
            case 's': {
                seed = atoi(optarg);
                fprintf(stderr, "random seed: %d\n", seed);
                break;
            }
            default: {
                fprintf(stderr, "error optopt: %c\n", optopt);
                fprintf(stderr, "error opterr: %d\n", opterr);
                exit(EXIT_FAILURE);
            }
        }
    }
    CompressGen* compressGenObj = new CompressGen(3.0, 3.0, seed);
    
    ofstream outputFile;
    outputFile.open(outputFileName, ios_base::binary | ios_base::trunc);

    size_t bufferSize = 1024 * 16;
    uint8_t* randomBuffer = (uint8_t*) malloc(bufferSize);
    uint8_t tmpChunk[MAX_CHUNK_SIZE];
    size_t compressedSize = 0;
    for (size_t i = 0; i < (totalProcessSizeMiB * 1024 * 1024) / bufferSize; i++) {
        compressGenObj->GenerateCompressibleData(randomBuffer, 2.0, bufferSize);
        outputFile.write((char*)randomBuffer, bufferSize);
        // for test the syn compression performance
        size_t count = bufferSize / MAX_CHUNK_SIZE;
        for (size_t j = 0; j < count; j++) {
            gettimeofday(&sTime, NULL);
            int ret = LZ4_compress_default((char*)(randomBuffer + j * MAX_CHUNK_SIZE), 
                (char*)tmpChunk, MAX_CHUNK_SIZE, MAX_CHUNK_SIZE);
            gettimeofday(&eTime, NULL);
            compressTime_ += tool::GetTimeDiff(sTime, eTime);
            if (ret != 0) {
                // compression success
                compressedSize += ret;
            } else {
                // cannot be compressed
                compressedSize += MAX_CHUNK_SIZE;
            }
        }
    }
    free(randomBuffer);

    fprintf(stderr, "compressed size (MiB): %lf\n", compressedSize / 1024.0 / 1024.0);
    fprintf(stderr, "total process size (MiB): %lu\n", totalProcessSizeMiB);
    outputFile.flush();
    outputFile.close();
    delete cryptoTool;
    EVP_MD_CTX_free(mdCtx);
    delete compressGenObj; 

    return 0;
}
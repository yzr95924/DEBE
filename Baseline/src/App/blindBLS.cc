/**
 * @file blindBLS.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the local blind-BLS
 * @version 0.1
 * @date 2021-03-31
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/cryptoPrimitive.h"
#include "../../include/define.h"
#include "../../include/constVar.h"

// for pbc
#include <pbc/pbc.h>
#include <pbc/pbc_test.h>

Configure config("config.json");
CryptoPrimitive* cryptoObj;
uint64_t totalChunkNum = 0;
uint64_t totalDataSize = 0;
double totalProcessTime = 0;
double initTime = 0;

// tmp bufffer
uint8_t* tmpCipherChunk;
uint8_t sessionKey[CHUNK_ENCRYPT_KEY_SIZE] = {1};
uint8_t chunkKey[CHUNK_ENCRYPT_KEY_SIZE];
uint8_t tmpHash[CHUNK_HASH_SIZE];

// for blind-BLS
pairing_t pairing;
element_t g1, g2, h, bh, r, k;
element_t x1, y2, sig;
element_t secret_key;
element_t temp1, temp2;

void BlindBLS(uint8_t* inputChunk, int chunkSize) {
    // step-1: compute the chunk hash
    cryptoObj->GenerateHash(inputChunk, chunkSize, tmpHash);
    // step-2-1: blind-BLS signature
    element_from_hash(h, tmpHash, CHUNK_HASH_SIZE);
    element_init_G1(r, pairing);
    element_random(r);
    element_init_G1(bh, pairing);
    element_init_G1(k, pairing);
    element_pow_zn(k, g1, r);
    element_mul(bh, h, k);
    // step-2-2: h^secret_key is the signature
    element_pow_zn(sig, bh, secret_key);
    element_div(sig, sig, k);
    // step-2-3: verification part 1 temp1=e(h^(prikey),g)
    element_pairing(temp1, sig, g2);
    // step-2-4: verification part 2 temp2=e(h,g^(prikey))
    //should match above
    element_pairing(temp2, h, y2);
    element_cmp(temp1, temp2);
    memcpy(tmpHash, &temp2, sizeof(temp2));
    // step-3: encryption with the key
    cryptoObj->EncryptWithKey(inputChunk, chunkSize, tmpHash, tmpCipherChunk);
    // step-4: compute the hash over the ciphertext 
    cryptoObj->GenerateHash(tmpCipherChunk, chunkSize, tmpHash);

    element_clear(bh);
    element_clear(k);
    element_clear(r);
    return ;
}

void Usage() {
    fprintf(stderr, "./blindBLS -i [input file name] -s [chunk size].\n");
    return ;
}

int main(int argc, char* argv[])
{
    const char optString[] = "i:s:";
    int option;

    string inputFileName;
    int chunkSize;

    if (argc < sizeof(optString)) {
        fprintf(stderr, "Wrong argc: %d\n", argc);
        Usage();
        exit(EXIT_FAILURE);
    }

    while ((option = getopt(argc, argv, optString)) != -1) {
        switch (option) {
            case 'i': {
                inputFileName.assign(optarg);
                break;
            } 
            case 's': {
                chunkSize = atoi(optarg) * 1024;
                break;
            }
            case '?': {
                fprintf(stderr, "Error optopt: %c\n", optopt);
                fprintf(stderr, "Error opterr: %d\n", opterr);
                Usage();
                exit(EXIT_FAILURE);
            }
        }

    }

    uint8_t* chunkBuffer;
    chunkBuffer = (uint8_t*) malloc(sizeof(uint8_t) * chunkSize);
    tmpCipherChunk = (uint8_t*) malloc(sizeof(uint8_t) * chunkSize);

    ifstream inputFile;
    inputFile.open(inputFileName, ios_base::binary);
    struct timeval sTime;
    struct timeval eTime;

    if (!inputFile.is_open()) {
        fprintf(stderr, "Cannot open the input file.\n");
        exit(EXIT_FAILURE);
    }

    cryptoObj = new CryptoPrimitive(CIPHER_TYPE, HASH_TYPE);
    
    // init the group G1, G2, GT
    FILE* fp;
    fp = fopen("../key/RSAkey/a.param", "r");
    if (!fp) {
        fprintf(stderr, "Cannot open the param file.\n");
        exit(EXIT_FAILURE);
    }
    char s[16384];
    size_t count = fread(s, 1, 16384, fp);
    fclose(fp);
    pairing_init_set_buf(pairing, s, count);
    fprintf(stderr, "Finish the init.\n");
    element_init_G2(g2, pairing);
    element_init_G2(y2, pairing);
    element_init_G1(g1, pairing);
    element_init_G1(x1, pairing);
    element_init_G1(h, pairing);
    element_init_G1(sig, pairing);
    element_init_GT(temp1, pairing);
    element_init_GT(temp2, pairing);
    element_init_Zr(secret_key, pairing);

    //generate system parameters
    element_random(g1);
    element_random(g2);

    //generate private key
    element_random(secret_key);

    //compute corresponding public key
    element_pow_zn(x1, g1, secret_key);
    element_pow_zn(y2, g2, secret_key);

    bool isEnd = false;
    while (!isEnd) {
        inputFile.read((char*)chunkBuffer, chunkSize);
        isEnd = inputFile.eof();
        gettimeofday(&sTime, NULL);
        BlindBLS(chunkBuffer, chunkSize);
        gettimeofday(&eTime, NULL);
        totalProcessTime += tool::GetTimeDiff(sTime, eTime);
        totalChunkNum++;
    }

    delete cryptoObj;
    free(tmpCipherChunk);
    free(chunkBuffer);
    element_clear(sig);
    element_clear(x1);
    element_clear(y2);
    element_clear(secret_key);
    element_clear(g1);
    element_clear(g2);
    element_clear(h);
    element_clear(temp1);
    element_clear(temp2);
    pairing_clear(pairing);
    fprintf(stderr, "Total process time: %lf\n", totalProcessTime);
    fprintf(stderr, "Blind-BLS Throughput: %lf\n", static_cast<double>(totalChunkNum * chunkSize) / (1024.0 * 1024.0 * totalProcessTime));
    return 0;
}

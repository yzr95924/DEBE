/**
 * @file blindRSA.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief implement the local Blind-RSA
 * @version 0.1
 * @date 2021-03-30
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/cryptoPrimitive.h"
#include "../../include/define.h"
#include "../../include/constVar.h"

// for blind RSA
#include <openssl/rsa.h>
#include <openssl/pem.h>

#define KEYMANGER_PRIVATE_KEY "../key/RSAkey/server.key"
#define KEYMANGER_PUBLIC_KEY_FILE "../key/RSAkey/serverpub.key"

using namespace std;

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


// for blind RSA
const BIGNUM* serverKeyN;
const BIGNUM* serverKeyD;
const BIGNUM* clientKeyN;
const BIGNUM* clientKeyE;
BN_CTX* bnCTX;

EVP_MD_CTX* mdCtx;
EVP_CIPHER_CTX* cipherCtx;


string Decoration(BIGNUM* r, u_char* hash) {
    BIGNUM *tmp = BN_new(), *h = BN_new();
    char result[128];
    memset(result, 0, sizeof(result));
    BN_bin2bn(hash, 32, h);
    //tmp=hash*r^e mod n
    BN_mod_exp(tmp, r, clientKeyE, clientKeyN, bnCTX);
    BN_mod_mul(tmp, h, tmp, clientKeyN, bnCTX);
    BN_bn2bin(tmp, (unsigned char*)result + (128 - BN_num_bytes(tmp)));
    string resultStr(result, 128);
    BN_free(tmp);
    BN_free(h);
    return resultStr;
}

string Elimination(BIGNUM* inv, u_char* key) {
    BIGNUM *tmp = BN_new(), *h = BN_new();
    char result[128];
    memset(result, 0, sizeof(result));
    BN_bin2bn(key, 128, h);
    //tmp=key*r^-1 mod n
    BN_mod_mul(tmp, h, inv, clientKeyN, bnCTX);
    BN_bn2bin(tmp, (unsigned char*)result + (128 - BN_num_bytes(tmp)));
    string resultStr(result, 128);
    BN_free(tmp);
    BN_free(h);
    return resultStr;
}

string KeyGen(std::string hash) {
    BIGNUM* result = BN_new();
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    BN_bin2bn((const unsigned char*)hash.c_str(), 128, result);
    //result=hash^d
    BN_mod_exp(result, result, serverKeyD, serverKeyN, bnCTX);
    BN_bn2bin(result, (unsigned char*)buffer + (128 - BN_num_bytes(result)));
    string key(buffer, 128);
    BN_free(result);
    return key;
}



void BlindRSA(uint8_t* chunkBuffer, int dataSize) {
    // step-1: compute the chunk hash
    cryptoObj->GenerateHash(mdCtx, chunkBuffer, dataSize, tmpHash);
    // step-2-1: blind-RSA, pick a random
    BIGNUM* r = BN_new();
    BIGNUM* inv = BN_new();
    BN_pseudo_rand(r, 256, -1, 0);
    BN_mod_inverse(inv, r, clientKeyN, bnCTX);
    // step-2-2: blind-RSA, decoration the chunk hash
    string key;
    key = Decoration(r, tmpHash);
    // step-2-3: generate the key
    string buffer;
    buffer = KeyGen(key);
    // step-2-4: eliminate the blind
    key = Elimination(inv, (u_char*)&buffer[0]);
    // step-3: encrypt with the key
    tool::PrintBinaryArray((uint8_t*)&key[0], key.size());
    cout << "key size: " << key.size() << endl;

    cryptoObj->EncryptWithKey(cipherCtx, chunkBuffer, dataSize, (uint8_t*)&key[0],
        tmpCipherChunk);
    // step-4: compute the hash over the ciphertext chunk
    cryptoObj->GenerateHash(mdCtx, tmpCipherChunk, dataSize, tmpHash);
    // clear all variable
    BN_free(r);
    BN_free(inv);

    return ;
}

void Usage() {
    fprintf(stderr, "./blindRSA -i [input file name] -s [chunk size].\n");
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

    mdCtx = EVP_MD_CTX_new();
    cipherCtx = EVP_CIPHER_CTX_new();

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

    // init the blind-RSA related variable
    gettimeofday(&sTime, NULL);
    RSA* serverRSA;
    BIO* priKeyFile = BIO_new_file(KEYMANGER_PRIVATE_KEY, "r");
    serverRSA = RSA_new();
    char passwd[5] = "1111";
    passwd[4] = '\0';
    PEM_read_bio_RSAPrivateKey(priKeyFile, &serverRSA, NULL, passwd);
    RSA_get0_key(serverRSA, &serverKeyN, NULL, &serverKeyD);

    RSA* clientRSA;
    BIO* pubKeyFile = BIO_new_file(KEYMANGER_PUBLIC_KEY_FILE, "r");
    if (!pubKeyFile) {
        fprintf(stderr, "Can not open key manager public key file.\n");
        exit(EXIT_FAILURE);
    }
    EVP_PKEY* pubKey = PEM_read_bio_PUBKEY(pubKeyFile, NULL, NULL, NULL);
    if (!pubKey) {
        fprintf(stderr, "Key manager public keyfile damage.\n");
    }
    clientRSA = EVP_PKEY_get1_RSA(pubKey);
    RSA_get0_key(clientRSA, &clientKeyN, &clientKeyE, NULL);

    BIO_free_all(pubKeyFile);
    BIO_free_all(priKeyFile);
    gettimeofday(&eTime, NULL);
    initTime = tool::GetTimeDiff(sTime, eTime);

    // finish the init of blind-RSA
    bnCTX = BN_CTX_new();
    bool isEnd = false;
    while (true) {
        inputFile.read((char*)chunkBuffer, chunkSize);
        isEnd = inputFile.eof();
        if (isEnd == true) {
            break;
        }
        gettimeofday(&sTime, NULL);
        BlindRSA(chunkBuffer, chunkSize);
        gettimeofday(&eTime, NULL);
        totalProcessTime += tool::GetTimeDiff(sTime, eTime);
        totalChunkNum++;
    }


    // clear openssl
    RSA_free(serverRSA);
    RSA_free(clientRSA);
    BN_CTX_free(bnCTX);
    EVP_PKEY_free(pubKey);

    EVP_MD_CTX_free(mdCtx);
    EVP_CIPHER_CTX_free(cipherCtx);

    delete cryptoObj;
    free(tmpCipherChunk);
    free(chunkBuffer);
    fprintf(stderr, "Init time: %lf\n", initTime);
    fprintf(stderr, "Total process time: %lf\n", totalProcessTime);
    fprintf(stderr, "Total chunk: %lu\n", totalChunkNum);
    fprintf(stderr, "Blind-RSA Throughput: %lf\n", static_cast<double>(totalChunkNum * chunkSize) / (1024.0 * 1024.0 * totalProcessTime));

    return 0;
}

/**
 * @file main.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief index simulator
 * @version 0.1
 * @date 2020-10-14
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "../../include/absIndex.h"
#include "../../include/freqIndex.h"
#include "../../include/tedIndex.h"

enum METHOD_TYPE {DEBE = 0, TED};

void Usage() {
    fprintf(stderr, "./KLDMain -i [input file] -m [method] -k [top-k threshold (K)] -t [trace type]\n");
    fprintf(stderr, "-m: method:\n");
    fprintf(stderr, "\t0: DEBE\n");
    fprintf(stderr, "\t1: TED\n");
    fprintf(stderr, "-t: trace type:\n");
    fprintf(stderr, "\t0: DOCKER, LINUX, VM, and FSL\n");
    fprintf(stderr, "\t1: MS\n");
}

string myName = "Sim";

int main(int argc, char* argv[]) {
    const char optString[] = "i:m:k:t:";
    int option;

    if (argc < sizeof(optString)) {
        tool::Logging(myName.c_str(), "wrong argc: %d\n", argc);
        Usage();
        exit(EXIT_FAILURE);
    }

    uint32_t k;
    int method;
    string inputFilePath;
    int traceType = FSL;
    while ((option = getopt(argc, argv, optString)) != -1) {
        switch (option) {
            case 'i': {
                inputFilePath.assign(optarg);
                break;
            }
            case 'm': {
                switch (atoi(optarg)) {
                    case DEBE: {
                        method = DEBE;
                        break;
                    }
                    case TED: {
                        method = TED;
                        break;
                    }
                    default: {
                        tool::Logging(myName.c_str(), "wrong method type.\n");
                        Usage();
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            }
            case 'k': {
                k = atoi(optarg);
                break;
            }
            case 't': {
                switch (atoi(optarg)) {
                    case FSL: {
                        traceType = FSL;
                        break;
                    }
                    case MS: {
                        traceType = MS;
                        break;
                    }
                    default: {
                        tool::Logging(myName.c_str(), "wrong trace type.\n");
                        Usage();
                        exit(EXIT_FAILURE);
                    }
                }
                break;
            }
            case '?': {
                tool::Logging(myName.c_str(), "error optopt: %c\n", optopt);
                tool::Logging(myName.c_str(), "error opterr: %d\n", opterr);
                Usage();
                exit(EXIT_FAILURE);
            }
        }
    } 
    tool::Logging(myName.c_str(), "--------config--------\n");
    tool::Logging(myName.c_str(), "input file path: %s\n", inputFilePath.c_str());
    tool::Logging(myName.c_str(), "threshold: %u K\n", k);
    tool::Logging(myName.c_str(), "----------------------\n");   


    AbsIndex* index; 
    switch (method) {
        case DEBE: {
            k *= 1024; 
            index = new FreqIndex(inputFilePath, traceType, k);
            break;
        }
        case TED: {
            index = new TEDIndex(inputFilePath, traceType);
            break;
        }
    }
    struct timeval sTime, eTime;
    gettimeofday(&sTime, NULL);
    index->ProcessTrace();
    gettimeofday(&eTime, NULL);
    double totalTime = tool::GetTimeDiff(sTime, eTime);
    tool::Logging(myName.c_str(), "total running time: %lf\n", totalTime);
    delete index;
    return 0;
}
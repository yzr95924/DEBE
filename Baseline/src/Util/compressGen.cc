/**
 * @file compressGen.cc
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief LZ data generator
 * @version 0.1
 * @date 2021-09-04
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "../../include/compressGen.h"

/**
 * @brief Construct a new Compress Gen object
 * 
 * @param len_exp exponent used for distribution of lengths
 * @param lit_exp exponent used for distribution of literals
 * @param seed the random seed
 */
CompressGen::CompressGen(double len_exp, double lit_exp, int seed) {
    len_exp_ = len_exp;
    lit_exp_ = lit_exp;
    // fix the seed as "95924"
    this->pcg32_srandom(seed, 0xC0FFEE);

	uint32_t initVal = 10;
	double currentRatio = 1.0;
	for (size_t i = 0; i < COMPRESSION_SET_SIZE; i++) {
		uint8_t* chunkBuffer = (uint8_t*) malloc(MAX_CHUNK_SIZE);
		this->GenerateCompressibleData(chunkBuffer, currentRatio + 0.1, MAX_CHUNK_SIZE);
		compressBlockSet_[initVal] = chunkBuffer;
		initVal++;
		currentRatio += 0.1;
	}
}

/**
 * @brief Destroy the Compress Gen object
 * 
 */
CompressGen::~CompressGen() {
	for (auto it = compressBlockSet_.begin(); it != compressBlockSet_.end(); it++) {
		free(it->second);
	}
}

/**
 * @brief generate random literals 
 * 
 * @param ptr the pointer to the where to store literals
 * @param size the number of literal to generate
 */
void CompressGen::GenerateLiterals(uint8_t* ptr, size_t size) {
    size_t i;
	for (i = 0; i < size; ++i) {
		ptr[i] = (uint8_t) (256 * pow(this->RandDouble(), lit_exp_));
	}
    return ;
}

/**
 * @brief gnerate random lengths
 * Generate length frequencies following a power distribution. If `len_exp` is
 * 1.0, the distribution is linear. As `len_exp` grows, the likelihood of small
 * values increases.
 * 
 * @param len_freq pointers to where to store length frequencies
 * @param num the number of the lengths to generate
 */
void CompressGen::GenerateLengths(uint32_t* len_freq, size_t num) {
    size_t i;
	for (i = 0; i < NUM_LEN; ++i) {
		len_freq[i] = 0;
	}

	for (i = 0; i < num; ++i) {
		size_t len = (size_t) (NUM_LEN * pow(this->RandDouble(), len_exp_));
        if ((len < NUM_LEN) == false) {
            fprintf(stderr, "CompressGen: len >= %d\n", NUM_LEN);
            exit(EXIT_FAILURE);
        }
		len_freq[len]++;
	}

    return ;
}

/**
 * @brief generate compressible data
 * 
 * @param ptr pointer to where to store generated data
 * @param size number of bytes to generate
 * @param ratio desired compression ratio
 */
void CompressGen::LZGenerateData(uint8_t* ptr, size_t size, double ratio) {
    uint32_t len_freq[NUM_LEN];
	uint8_t buffer[MAX_LEN];
	uint8_t* p = ptr;
	size_t cur_len = 0;
	size_t i = 0;
	int last_was_match = 0;
	ratio += 0.15;

	len_freq[0] = 0;

	while (i < size) {
		size_t len;
		/* Find next length with non-zero frequency */
		while (len_freq[cur_len] == 0) {
			if (cur_len == 0) {
				this->GenerateLiterals(buffer, MAX_LEN);
				this->GenerateLengths(len_freq, LEN_PER_CHUNK);
				cur_len = NUM_LEN;
			}
			cur_len--;
		}
		len = MIN_LEN + cur_len;
		len_freq[cur_len]--;
		if (len > size - i) {
			len = size - i;
		}
		if (this->RandDouble() < 1.0 / ratio) {
			/* Insert len literals */
			this->GenerateLiterals(p, len);
			last_was_match = 0;
		}
		else {
			/* Insert literal to break up matches */
			if (last_was_match) {
				this->GenerateLiterals(p, 1);
				i++;
				p++;
				if (len > size - i) {
					len = size - i;
				}
			}
			/* Insert match of length len */
			memcpy(p, buffer, len);
			last_was_match = 1;
		}
		i += len;
		p += len;
	}
}

/**
 * @brief generate the compressible data
 * 
 * @param buffer the buffer to store the 
 * @param compressionRatio the compression ratio
 * @param size the expected chunk size
 */
void CompressGen::GenerateCompressibleData(uint8_t* buffer, double compressionRatio, size_t size) {
    this->LZGenerateData(buffer, size, compressionRatio);
    return ;
}

/**
 * @brief generate the compressible chunk from the candidate set
 * 
 * @param chunkBuffer the pointer to the chunk buffer
 * @param compressionInt the compression ratio int
 * @param chunkSize the chunk size
 */
void CompressGen::GenerateChunkFromCanditdateSet(uint8_t* chunkBuffer, uint32_t compressionInt, size_t chunkSize) {
    if (compressionInt > 30) {
        compressionInt = 30;
    }
    if (compressionInt < 10) {
        compressionInt = 10;
    }
    uint8_t* compressedChunkBuffer = compressBlockSet_[compressionInt];
    memcpy(chunkBuffer, compressedChunkBuffer, chunkSize);
    return ;
}
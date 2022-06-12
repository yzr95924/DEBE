/**
 * @file define.h
 * @author Zuoru YANG (zryang@cse.cuhk.edu.hk)
 * @brief include the necessary header 
 * @version 0.1
 * @date 2020-09-09
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef MY_DEFINE_H
#define MY_DEFINE_H

#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <iomanip>
#include <bits/stdc++.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>

static const uint64_t MB_2_B = 1000 * 1000;
static const uint64_t MiB_2_B = uint64_t(1) << 20;
static const uint64_t KB_2_B = 1000;
static const uint64_t KiB_2_B = uint64_t(1) << 10;
static const uint64_t SEC_2_US = 1000 * 1000;

static const char ALPHABET[] = {'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l',
                                'm', 'n', 'o', 'p', 'q', 'r',
                                's', 't', 'u', 'v', 'w', 'x',
                                'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9'};

namespace tool {
    /**
     * @brief Get the Time Diff object
     * 
     * @param start_time start time
     * @param end_time end time
     * @return double the diff time (sec)
     */
    inline double GetTimeDiff(struct timeval start_time, struct timeval end_time) {
        double second;
        second = static_cast<double>(end_time.tv_sec - start_time.tv_sec) * SEC_2_US + 
            end_time.tv_usec - start_time.tv_usec;
        second = second / SEC_2_US;
        return second; 
    }
    
    /**
     * @brief compare the limits with the input
     * 
     * @param input the input number
     * @param lower the lower bound of the limitation
     * @param upper the upper bound of the limitation
     * @return uint32_t 
     */
    inline uint32_t CompareLimit(uint32_t input, uint32_t lower, uint32_t upper) {
        if (input <= lower) {
            return lower; 
        } else if (input >= upper) {
            return upper;
        } else {
            return input;
        }    
    }
    
    /**
     * @brief get the ceil of the division
     * 
     * @param a 
     * @param b 
     * @return uint32_t 
     */
    inline uint32_t DivCeil(uint32_t a, uint32_t b) {
        uint32_t tmp = a / b;
        if (a % b == 0) {
            return tmp;
        } else {
            return (tmp + 1);
        }
    }
    
    /**
     * @brief print the binary buffer
     * 
     * @param fp the pointer to the buffer
     * @param fp_size the size of the buffer
     */
    inline void PrintBinaryArray(const uint8_t* buffer, size_t buffer_size) {
        for (size_t i = 0; i < buffer_size; i++) {
            fprintf(stdout, "%02x", buffer[i]);
        }
        fprintf(stdout, "\n");
        return ;
    }
    
    /**
     * @brief a simple logger
     * 
     * @param logger the logger name
     * @param fmt the input message
     */
    inline void Logging(const char* logger, const char* fmt, ...) {
        using namespace std;
        char buf[BUFSIZ] = {'\0'};
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, BUFSIZ, fmt, ap);
        va_end(ap);
        time_t t = std::time(nullptr);
        stringstream output;
        output << std::put_time(std::localtime(&t), "%F %T ")
            << "<" << logger << ">: " << buf;
        cerr << output.str();
        return ;
    }

    inline uint64_t ProcessMemUsage() {
        using std::ios_base;
        using std::ifstream;
        using std::string;

        uint64_t vm_usage     = 0;
        uint64_t resident_set = 0;

        // 'file' stat seems to give the most reliable results
        //
        ifstream stat_stream("/proc/self/stat",ios_base::in);

        // dummy vars for leading entries in stat that we don't care about
        //
        string pid, comm, state, ppid, pgrp, session, tty_nr;
        string tpgid, flags, minflt, cminflt, majflt, cmajflt;
        string utime, stime, cutime, cstime, priority, nice;
        string O, itrealvalue, starttime;

        // the two fields we want
        //
        unsigned long vsize;
        long rss;

        stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
                    >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
                    >> utime >> stime >> cutime >> cstime >> priority >> nice
                    >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

        stat_stream.close();

        long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
        vm_usage     = vsize / 1024 ;
        resident_set = rss * page_size_kb;
        return resident_set; // only for PM
    }

    inline uint64_t GetMaxMemoryUsage() {
        struct rusage currentUsage;
        getrusage(RUSAGE_SELF, &currentUsage);
        return currentUsage.ru_maxrss;
    }

    inline void CreateUUID(char* buffer, size_t idLen) {
        for (size_t i = 0; i < idLen; i++) {
            char* pos = buffer + i;
            *pos = ALPHABET[rand() % sizeof(ALPHABET)];
        }
        return ;
    }

    inline bool FileExist(std::string filePath) {
        return std::filesystem::is_regular_file(filePath);
    }

    inline uint64_t GetStrongSeed() {
        
        uint64_t a = clock();
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        uint64_t b = currentTime.tv_sec * SEC_2_US + currentTime.tv_usec;
        uint64_t c = getpid();

        // Robert Jenkins' 96 bit Mix Function
        a = a - b;  a = a - c;  a = a ^ (c >> 13);
        b = b - c;  b = b - a;  b = b ^ (a << 8);
        c = c - a;  c = c - b;  c = c ^ (b >> 13);
        a = a - b;  a = a - c;  a = a ^ (c >> 12);
        b = b - c;  b = b - a;  b = b ^ (a << 16);
        c = c - a;  c = c - b;  c = c ^ (b >> 5);
        a = a - b;  a = a - c;  a = a ^ (c >> 3);
        b = b - c;  b = b - a;  b = b ^ (a << 10);
        c = c - a;  c = c - b;  c = c ^ (b >> 15);

        return c;
    }

} // namespace tool
#endif 
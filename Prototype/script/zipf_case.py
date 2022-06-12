from os import replace
import random
from tqdm import tqdm
import getopt
import sys
import time

alphabet_set = ['0', '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'a', 'b', 'c', 'd', 'e', 'f']
fp_size = 6
chunk_size = 8192

def Usage():
    print("{name} -s [total_chunk_number] -d [deduplication factor] -o [output file] -c [zpif constant]".format(name=__file__))

def ZipfNormalizedFreq(zipf_divider, k, s):
    freq = 1 / (k**s)
    freq = freq / zipf_divider
    return freq


def ComputeZipfDivider(n, s):
    zipf_divider = 0
    for i in range(1, n+1):
        tmp = 1 / (i**s)
        zipf_divider = zipf_divider + tmp
    return zipf_divider

def GenerateUniqueChunkList(unique_chunk_list: list, unique_chunk_num):
    for i in tqdm(range(unique_chunk_num)):
        chunk_fp = ""
        for item in range(fp_size):
            chunk_fp = chunk_fp + alphabet_set[random.randrange(0, len(alphabet_set))]
            chunk_fp = chunk_fp + alphabet_set[random.randrange(0, len(alphabet_set))]
            chunk_fp = chunk_fp + ":"
        chunk_fp = chunk_fp[:-1]
        unique_chunk_list.append(chunk_fp)
    print("Generate the unique data set done!")

def GenerateLogicalChunkList(output_file_name: str, unique_chunk_list: list, logical_chunk_num, freq_list: list):
    output_file = open(output_file_name, mode='w')
    for i in tqdm(range(logical_chunk_num)):
        if (i < len(unique_chunk_list)):
            output_file.write(unique_chunk_list[i] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
        else:
            output_file.write(random.choices(population=unique_chunk_list, weights=freq_list)[0] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
    output_file.close()
    print("Output the trace file done!")

def GenerateLogicalChunkListNew(output_file_name: str, unique_chunk_list: list, 
    logical_chunk_num, freq_list: list):
    output_file = open(output_file_name, mode='w')
    for i in range(len(unique_chunk_list)):
        output_file.write(unique_chunk_list[i] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
    it_time = logical_chunk_num / len(unique_chunk_list) - 1
    for i in tqdm(range(int(it_time))):
        chunk_fp_list = random.choices(population=unique_chunk_list, weights=freq_list, k=len(unique_chunk_list))
        for item in range(len(chunk_fp_list)):
            output_file.write(chunk_fp_list[item] + "\t\t" + str(chunk_size) + "\t\t10" + "\n")
    output_file.close()
    print("Output the trace file done!")
            
if __name__ == "__main__":
    logical_chunk_num = 0
    dedup_factor = 0
    unique_chunk_num = 0
    output_file_name = ""
    a = 1
    opts, args = getopt.getopt(sys.argv[1:], "-s:-c:-d:-o:-h")
    for opt_name, opt_value in opts:
        if opt_name == '-s':
            logical_chunk_num = int(opt_value)
        elif opt_name == '-d':
            dedup_factor = int(opt_value)
        elif opt_name == '-c':
            a = float(opt_value)
            print(a)
        elif opt_name == '-h':
            Usage()
            exit()
        elif opt_name == '-o':
            output_file_name = opt_value
        else:
            Usage()
            exit()

    unique_chunk_num = int(logical_chunk_num / dedup_factor)
    random.seed(time.time())
    zipf_divider = ComputeZipfDivider(unique_chunk_num, a)

    freq_list = []
    print("Generate the freq distribution")
    for i in tqdm(range(1, unique_chunk_num+1)):
        tmp_freq = ZipfNormalizedFreq(zipf_divider, i, a);
        freq_list.append(tmp_freq)
    print("sum: {sum_val}".format(sum_val=sum(freq_list)))

    fp_list = []
    print("Generate the chunk fingerprint")
    GenerateUniqueChunkList(fp_list, unique_chunk_num)

    start_time = time.time()
    GenerateLogicalChunkListNew(output_file_name, fp_list, logical_chunk_num, freq_list)
    total_time = time.time() - start_time
    print("Total running time: {total_time}".format(total_time=total_time))
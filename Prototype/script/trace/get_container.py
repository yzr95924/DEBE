# script for downloading & processing the couchbase docker container traces

import os

# from community-2.2.0 to the end, waiting for process
version_set = ["7.0.1","enterprise-7.0.0-beta","enterprise-6.6.2","enterprise-6.6.1","enterprise-6.6.0","enterprise-6.5.1","enterprise-6.5.0","enterprise-6.0.5","enterprise-6.0.4","enterprise-6.0.3",\
"enterprise-6.0.2","enterprise-6.0.1","enterprise-7.0.1","community-7.0.0-beta","community-6.6.0","community-6.5.1","community-6.5.0",\
"community-7.0.1","7.0.0-beta","6.6.2","6.6.1","6.6.0","6.5.1","6.5.0","6.0.5","6.0.4","6.0.3","6.0.2","6.0.1","community-6.0.0","6.5.0-beta2",\
"6.5.0-beta","enterprise-6.0.0","6.0.0","community-4.5.1","community-5.1.1","enterprise-4.6.5","4.6.5","6.0.0-beta","enterprise-5.5.2","5.5.2",\
"enterprise-5.5.1","5.5.1","enterprise-5.5.0","5.5.0","5.5.0-beta","enterprise-5.1.1","5.1.1","community-5.0.1","enterprise-4.6.4","4.6.4",\
"enterprise-5.1.0","5.1.0","enterprise-5.0.1","5.0.1","enterprise-4.6.3","4.6.3","community-3.1.3","enterprise-3.1.6","3.1.6",\
"enterprise-4.6.2","4.6.2","enterprise-4.6.1","4.6.1","community-4.5.0","enterprise-4.6.0","4.6.0","enterprise-4.5.1","4.5.1","community-4.1.1",\
"community-4.1.0","enterprise-4.5.0","4.5.0","community-4.0.0","enterprise-3.1.5","3.1.5","enterprise-4.1.1","4.1.1","community-2.2.0",\
"enterprise-2.5.2","2.5.2","community-3.0.1","enterprise-3.0.2","3.0.2","enterprise-3.0.3","3.0.3","enterprise-3.1.0","3.1.0","enterprise-3.1.3",\
"3.1.3","enterprise-4.0.0","4.0.0","enterprise-4.1.0","4.1.0"]

# for test
# version_set = ["7.0.1"]

home_dir = "" # save the trace in your $HOME dir
package_flag = "-cvf"
download_output_folder = "" # for download output folder
repack_output_folder = "" # for packed trace output folder
download_script_path = "./script/trace/download-frozen-image-v2.sh" # the script of download third party
# fs_hasher_path = ""
# fs_stat_path = ""
# hash_file_path = ""
# trace_path = ""

def PrepareTraceFolder():
    print("Prepare the folder to store the container trace")
    global home_dir
    global download_output_folder
    global repack_output_folder
    # global fs_hasher_path
    # global hash_file_path
    # global fs_stat_path
    # global trace_path

    home_dir = os.path.expanduser('~')
    download_output_folder = os.path.join(home_dir, "CONTAINER_trace/download_trace/")
    repack_output_folder = os.path.join(home_dir, "CONTAINER_trace/")
    # fs_hasher_path = os.path.join(home_dir, "fs-hasher-0.9.5/fs-hasher")
    # FS_STAT_PATH = os.path.join(home_dir, "fs-hasher-0.9.5/hf-stat")
    # hash_file_path = os.path.join(home_dir, "container-trace/tar_hash")
    # trace_path = os.path.join(home_dir, "container-trace/trace_hash")

    cmd = "mkdir -p " + download_output_folder
    print(cmd)
    os.system(cmd)
    cmd = "mkdir -p " + repack_output_folder
    print(cmd)
    os.system(cmd)
    # cmd = "mkdir -p " + hash_file_path
    # print(cmd)
    # os.system(cmd)
    # cmd = "mkdir -p " + trace_path
    # print(cmd)
    # os.system(cmd)
    print("Done")

def Download(input_version):
    # format example: bash download-frozen-image-v2.sh tst couchbase:enterprise-6.6.1
    cmd = "bash " + download_script_path + " " + download_output_folder + " " + "couchbase:" + input_version 
    print(cmd)
    os.system(cmd)
    # decompress all layer.tar in the sub-folder
    for sub_file in os.listdir(download_output_folder):
        sub_dir = os.path.join(download_output_folder, sub_file)
        if (os.path.isdir(sub_dir)):
            # check whether it has the sub-folder
            for sub_sub_file in os.listdir(sub_dir):
                if (sub_sub_file == "layer.tar"):
                    sub_sub_file_path = os.path.join(sub_dir, sub_sub_file)
                    cmd = "tar -xvf " + sub_sub_file_path + " -C " + sub_dir
                    print(cmd)
                    os.system(cmd)
                    cmd = "rm " + sub_sub_file_path
                    print(cmd)
                    os.system(cmd)

def PackTar(input_version):
    output_file_name = os.path.join(repack_output_folder, input_version + ".tar")
    cmd = "tar " + package_flag + " " + output_file_name + " " + download_output_folder
    print(cmd)
    os.system(cmd)

    delete_folder = os.path.join(download_output_folder, "*")
    cmd = "rm -rf " + delete_folder
    print(cmd)
    os.system(cmd)

# def GenerateChunkFp(input_version):
#     input_file_path = os.path.join(repack_output_folder, input_version + ".tar")
#     output_hash_file = os.path.join(hash_file_path, input_version + ".hash")
#     # format example:
#     # ./fs-hasher/bin/fs-hasher -p ./packed_enterprises/enterprise-6.6.1 -c variable -h md5-48bit -z none -o ./hash_files/enterprise-6.6.1.hash
#     cmd = fs_hasher_path + " -p " + input_file_path + " -c variable -h md5-48bit -z none -o " + output_hash_file
#     print(cmd)
#     os.system(cmd)

#     output_final_hash_file = os.path.join(trace_path, input_version + "-hash")
#     # format example:
#     # ./../fs-hasher/bin/fs-stat -h ./packed_enterprises/enterprise-6.6.1 > ./traces/enterprise-6.6.1
#     cmd = fs_stat_path + " -h " + output_hash_file + " > " + output_final_hash_file
#     print(cmd)
#     os.system(cmd)

#     cmd = "sed -i '1d' " + output_final_hash_file
#     print(cmd)
#     os.system(cmd)

#     cmd = "rm " + output_hash_file
#     print(cmd)
#     os.system(cmd)

def CleanUp():
    print("Clean up")
    # cmd = "rm -rf " + hash_file_path
    # print(cmd)
    # os.system(cmd)

    cmd = "rm -rf " + download_output_folder
    print(cmd)
    os.system(cmd)

if __name__ == "__main__":
    PrepareTraceFolder()
    for version in version_set:
        Download(version)
        PackTar(version)
        #GenerateChunkFp(version)
    CleanUp()
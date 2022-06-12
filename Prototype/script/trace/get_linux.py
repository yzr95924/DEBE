# script for downloading & processing the linux source code traces

import os

prefix = "https://github.com/torvalds/linux/archive/refs/tags/"

version_set = []
prefix_v5 = "5."
for i in range(0, 14): # 1-13
    version_set.append(prefix_v5 + str(i));
prefix_v4 = "4."
for i in range(0,21): # 0-20
    version_set.append(prefix_v4 + str(i))
prefix_v3 = "3."
for i in range(0, 20): # 0-19
    version_set.append(prefix_v3 + str(i))
prefix_v2_6 = "2.6."
for i in range(13, 40): # 13-40
    version_set += [prefix_v2_6 + str(i)]

# for test    
# version_set = ["5.5"]

home_dir = "" # save the trace in your $HOME dir
package_flag = "-cvf"
download_output_folder = "" # for download output folder
repack_output_folder = ""
# fs_hasher_path = ""
# fs_stat_path = ""
# hash_file_path = ""
# trace_path = ""

def PrepareTraceFolder():
    print("Prepare the folder to store the linux source")
    global home_dir
    global download_output_folder
    global repack_output_folder
    # global fs_hasher_path
    # global hash_file_path
    # global fs_stat_path
    # global trace_path

    home_dir = os.path.expanduser('~')
    download_output_folder = os.path.join(home_dir, "LINUX_trace/download_trace")
    repack_output_folder = os.path.join(home_dir, "LINUX_trace/")
    # fs_hasher_path = os.path.join(home_dir, "fs-hasher-0.9.5/fs-hasher")
    # fs_stat_path = os.path.join(home_dir, "fs-hasher-0.9.5/hf-stat")
    # hash_file_path = os.path.join(home_dir, "linux-trace/tar_hash")
    # trace_path = os.path.join(home_dir, "linux-trace/trace_hash")

    cmd = "mkdir -p " + download_output_folder
    os.system(cmd)
    cmd = "mkdir -p " + repack_output_folder
    os.system(cmd)
    # cmd = "mkdir -p " + hash_file_path
    # os.system(cmd)
    # cmd = "mkdir -p " + trace_path
    # os.system(cmd)
    print("Done")

def Download(input_version):
    # https://github.com/torvalds/linux/archive/refs/tags/v5.16.tar.gz
    download_file_name = "v" + input_version + ".tar.gz"
    input_link = prefix + download_file_name
    cmd = "wget -P " + download_output_folder + " " + input_link 
    print(cmd)
    os.system(cmd)
    download_file_path = os.path.join(download_output_folder, download_file_name)
    cmd = "tar -xvf " + download_file_path + " -C " + download_output_folder
    print(cmd)
    os.system(cmd)

    # delete the tar.gz
    cmd = "rm " + download_file_path
    print(cmd)
    os.system(cmd)

#tar xvf FileName.tar
def depack(input_version):
    depack_dir_name = repack_output_folder + "v" + input_version + "/"
    cmd = "tar -xvf " + depack_dir_name + "v" + input_version

def tar2mtar(input_version):
    # transfer tar to mtar
    #/usr/local/bin/tar --filter -f v5.4.tar
    #/usr/local/bin/tar --migrate -f v5.4.tar.f 
    # keep .tar.f.m & remove .tar .tar.f
    output_file_name = repack_output_folder + "v" + input_version + "/v" + input_version + ".tar"
    
    cmd = "/usr/local/bin/tar --filter -f " + output_file_name
    print(cmd)
    os.system(cmd)

    cmd = "/usr/local/bin/tar --migrate -f " + output_file_name + ".f"
    print(cmd)
    os.system(cmd)
    
    # remove the download file
    cmd = "rm -rf " + download_output_folder
    print(cmd)
    os.system(cmd)
    
    #remove extra file
    cmd = "rm " + output_file_name + ".f"
    print(cmd)
    os.system(cmd)
    cmd = "rm " + output_file_name
    print(cmd)
    os.system(cmd)

def PackTar(input_version):
    output_file_name = os.path.join(repack_output_folder, input_version + ".tar")
    download_file_folder = os.path.join(download_output_folder, "linux-" + input_version)
    cmd = "tar " + package_flag + " " + output_file_name + " " + download_file_folder
    print(cmd)
    os.system(cmd)

    delete_folder = os.path.join(download_output_folder, "*")
    cmd = "rm -rf " + delete_folder
    print(cmd)
    os.system(cmd)
    cmd = "/usr/local/bin/tar --filter -f " + output_file_name
    print(cmd)
    os.system(cmd)
    cmd = "/usr/local/bin/tar --migrate -f " + output_file_name + ".f"
    print(cmd)
    os.system(cmd)
    cmd = "rm " + output_file_name + ".f"
    print(cmd)
    os.system(cmd)
    cmd = "rm " + output_file_name
    print(cmd)
    os.system(cmd)

# def GenerateChunkFp(input_version):
#     input_file_path = os.path.join(repack_output_folder, input_version + ".tar.f.m")
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
    # os.system(cmd)
    cmd = "rm -rf " + download_output_folder
    os.system(cmd)

if __name__ == "__main__":
    PrepareTraceFolder()
    for version in version_set:
        Download(version)
        PackTar(version)
        # GenerateChunkFp(version)
    CleanUp()
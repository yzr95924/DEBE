import sys
import getopt
import os

exector = "./DEBEClient"

def Usage():
    print("{name} -i [the trace folder] -o [output script name] -t [u/d] "
        " -s [keep restore file] -f [using ramdisk] -d [ramdisk path]".format(name=__file__))
    print("-t: operation ([u/d]):")
    print("\tu: upload")
    print("\td: download")
    print("-s: restore files [keep restore files]")
    print("\t0: not keep restore files")
    print("\t1: keep restore files")
    print("-f: use ramdisk? [using ramdisk]")
    print("\t0: do not use ramdisk")
    print("\t1: use ramdisk")

def GetAllTrace(folder_name: str):
    file_list = sorted(os.listdir(folder_name))
    return file_list

if __name__ == "__main__":
    input_folder = ""
    output_script_name = ""
    operation_type = ""
    restore_file_stat = 0
    using_ramdisk = 0
    ramdisk_path = ""
    opts, args = getopt.getopt(sys.argv[1:], "-i:-o:-t:-m:-s:-f:-d:-h")
    for opt_name, opt_value in opts:
        if opt_name == '-i':
            input_folder = opt_value
        elif opt_name == '-o':
            output_script_name = opt_value 
        elif opt_name == '-t':
            operation_type = opt_value 
        elif opt_name == '-s':
            restore_file_stat = int(opt_value)
            if (restore_file_stat > 1):
                print("wrong [keep restore files] flag")
                Usage()
                exit()
        elif opt_name == '-f':
            using_ramdisk = int(opt_value)
            if (using_ramdisk > 1):
                print("wrong [using ramdisk] flag")
                Usage()
                exit()
        elif opt_name == '-d':
            ramdisk_path = opt_value
            if ((os.path.isdir(ramdisk_path) == False) and (using_ramdisk == 1)):
                print("invalid ramdisk path")
                Usage()
                exit()
        elif opt_name == '-h':
            Usage()
            exit()
        else:
            Usage()
            exit()

    trace_list = GetAllTrace(input_folder)
    output_script_file = open(output_script_name, 'w') 
    for trace in trace_list:
        if (using_ramdisk == 0):
            trace_full_path = os.path.join(input_folder, trace)
            cmd = exector + " -t " + operation_type + " -i " + trace_full_path
            output_script_file.write(cmd + "\n")
            if (operation_type == "d" and restore_file_stat == 0):
                cmd = "rm " + trace_full_path + "-d"
                output_script_file.write(cmd + "\n")
        else:
            trace_full_path = os.path.join(input_folder, trace)
            if (operation_type == "u"):
                # cp the input trace to the ramdisk
                cmd = "cp " + trace_full_path + " " + ramdisk_path
                output_script_file.write(cmd + "\n")
                trace_ramdisk_path = os.path.join(ramdisk_path, trace)
                cmd = exector + " -t " + operation_type + " -i " + trace_ramdisk_path
                # rm the input trace from the ramdisk
                output_script_file.write(cmd + "\n")
                cmd = "rm " + trace_ramdisk_path
                output_script_file.write(cmd + "\n")
            else:
                trace_ramdisk_path = os.path.join(ramdisk_path, trace)
                cmd = exector + " -t " + operation_type + " -i " + trace_ramdisk_path
                output_script_file.write(cmd + "\n")
                # do not keep the restore file in the ramdisk
                cmd = "rm " + trace_ramdisk_path + "-d"
                output_script_file.write(cmd + "\n")
    output_script_file.close()
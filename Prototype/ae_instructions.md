# AE Detailed Instructions
Here are the detailed instructions to perform the same experiments in our paper.

## Artifact claims

We claim that **the resultant numbers might be different** from in our paper due to various factors (e.g., different machines, different OS, different software packages...). Nevertheless, we expect that DEBE should still outperform the baseline approaches in terms of performance, storage efficiency, and security (i.e., our main results).

Also, to ensure the results are stable, you may need to disable the swap space in your machine.

## Evaluation on Synthetic Data

**Note that: before starting every experiment, please ensure that `./DEBE/Prototype/config.json` and `./DEBE/Baseline/config.json` are correctly configured.**

### Exp#1 (Overall performance)

- **step-1: preparation**

In the client machine:

```shell
# prepare a 6GiB ramdisk in "~/ram-test/"
$ sudo mount -t tmpfs -o rw,size=6G tmpfs ~/ram-test/

# prepare the synthetic trace (put it in the memory)
$ cd ./DEBE/Prototype/bin
# generate a 2GiB file (with the random seed as 1)
$ ./compressSYN -i 2048 -o ~/ram-test/test-2G-syn -s 1

$ cd ./DEBE/Prototype
$ bash ./script/exp1/exp1_pre.sh
$ cd ./DEBE/Baseline
$ bash ./recompile.sh
```

In the storage server machine:

```shell
# store the containers in the memory
$ cd ./DEBE/Prototype
$ sudo mount -t tmpfs -o rw,size=6G tmpfs ./bin
$ cd ./DEBE/Baseline
$ sudo mount -t tmpfs -o rw,size=6G tmpfs ./bin

$ cd ./DEBE/Prototype
$ bash ./script/exp1/exp1_pre.sh
$ cd ./DEBE/Baseline
$ bash ./recompile.sh
```

- **step-2: evaluate DEBE**

In the storage server machine:

```shell
# start the storage server
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -m 4
...
...
2022-05-10 01:48:19 <DEBEServer>: waiting the request from the client.
```

In the client machine:

```shell
$ cd ./DEBE/Prototype/bin
# first upload: all unique data
$ ./DEBEClient -t u -i ~/ram-test/test-2G-syn
# second upload: all duplicate data
$ ./DEBEClient -t u -i ~/ram-test/test-2G-syn
# download the file
$ ./DEBEClient -t d -i ~/ram-test/test-2G-syn

# check the result
$ cat client-log
input file, opt, logical data size (B), logical chunk num, total time (s), speed (MiB/s)
/home/zuoru/ram-test/test-2G-syn, upload, 2147483648, 226837, 9.671531, 211.755512
/home/zuoru/ram-test/test-2G-syn, upload, 2147483648, 226837, 6.915841, 296.131736
/home/zuoru/ram-test/test-2G-syn, download, 2147483648, 226837, 2.970476, 689.451791
```

Reset the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Prototype
$ base recompile.sh
# clean all stored containers in ./bin
```

- **step-3: evaluate the baseline approaches**

We use CE as an example to demonstrate the workflow: (you can vary the `-m` parameter for the client to test different baseline approaches, see the README file in `./DEBE/Baseline`)

In the storage server machine:

```shell
# start the storage server
$ cd ./DEBE/Baseline/bin
$ ./DAEServer
...
...
2022-05-10 02:16:17 <DAEServer>: waiting the request from the client.
```

In the client machine:

```shell
$ cd ./DEBE/Baseline/bin
# first upload: all unique data
$ ./DAEClient -t u -i ~/ram-test/test-2G-syn -m 1
# second upload: all duplicate data
$ ./DAEClient -t u -i ~/ram-test/test-2G-syn -m 1
# download the file
$ ./DEBEClient -t d -i ~/ram-test/test-2G-syn

# Note that: you can vary '-m' value to test different DaE approaches (see the README file in "./DEBE/Baseline") and you need to reset the storage server before testing a new approach

# check the result
$ cat client-log
input file, opt, logical data size (B), logical chunk num, total time (s), speed (MiB/s)
/home/zuoru/ram-test/test-2G-syn, upload, 2147483648, 226837, 11.8135851, 173.359736
/home/zuoru/ram-test/test-2G-syn, upload, 2147483648, 226837, 11.317041, 180.966031
/home/zuoru/ram-test/test-2G-syn, download, 2147483648, 226837, 2.792135, 733.488889
# can see that CE is lower than DEBE for upload performance
```

Note that you can vary the `-m` parameter for the client to test different baseline approaches. Also, **you need to reset the storage server before each test**)

Reset the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Baseline
$ base recompile.sh
# clean all stored containers in ./bin
```

- **step-4: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ~/ram-test/
```

In the storage server machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./DEBE/Baseline/bin 
$ sudo umount -l ./DEBE/Prototype/bin
```

### Exp#2 (Upload breakdown)

- **step-1: preparation**

In the client machine:

```shell
# prepare a 6GiB ramdisk in "~/ram-test/"
$ sudo mount -t tmpfs -o rw,size=6G tmpfs ~/ram-test/

# prepare the synthetic trace (put it in the memory)
$ cd ./DEBE/Prototype/bin
# generate a 2GiB file (with the random seed as 1)
$ ./compressSYN -i 2048 -o ~/ram-test/test-2G-syn -s 1

# enable breakdown time recording
$ cd ./DEBE/Prototype
$ bash ./script/exp2/exp2_pre.sh
```

In the storage server machine:

```shell
# store the containers in the memory
$ cd ./DEBE/Prototype
$ sudo mount -t tmpfs -o rw,size=6G tmpfs ./bin

$ cd ./DEBE/Prototype
$ bash ./script/exp2/exp2_pre.sh
```

- **step-2: evaluation**

In the storage server machine:

```shell
# start the storage server
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -m 4
...
...
2022-05-10 01:48:19 <DEBEServer>: waiting the request from the client.
```

In the client machine:

```shell
$ cd ./DEBE/Prototype/bin
# first upload: all unique data
$ ./DEBEClient -t u -i ~/ram-test/test-2G-syn

# check the chunking time breakdown
...
chunking time: 0.651941
...

# check the enclave time breakdown in the storage server machine
...
2022-05-10 00:14:11 <ServerOptThread>: data tran time: 0.358696
2022-05-10 00:14:11 <ServerOptThread>: fingerprint time: 2.321155
2022-05-10 00:14:11 <ServerOptThread>: freq counting time: 0.053411
2022-05-10 00:14:11 <ServerOptThread>: first-dedup time: 0.124025
2022-05-10 00:14:11 <ServerOptThread>: second-dedup time: 0.920500
2022-05-10 00:14:11 <ServerOptThread>: compression time: 0.660405
2022-05-10 00:14:11 <ServerOptThread>: encryption time: 0.298359
...

$ # second upload: all duplicate data
$ ./DEBEClient -t u -i ~/ram-test/test-2G-syn
# check the chunking time breakdown
...
chunking time: 0.624310
...

# check the enclave time breakdown in the storage server machine
...
2022-05-10 03:18:59 <ServerOptThread>: data tran time: 0.338574
2022-05-10 03:18:59 <ServerOptThread>: fingerprint time: 2.292659
2022-05-10 03:18:59 <ServerOptThread>: freq counting time: 0.045167
2022-05-10 03:18:59 <ServerOptThread>: first-dedup time: 0.148249
2022-05-10 03:18:59 <ServerOptThread>: second-dedup time: 0.000000
2022-05-10 03:18:59 <ServerOptThread>: compression time: 0.000000
2022-05-10 03:18:59 <ServerOptThread>: encryption time: 0.000000
...
```

Reset the storage server machine (**you need to reset the storage server before each test**):

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Prototype
$ base recompile.sh
# clean all stored containers in ./bin
```

- **step-3: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ~/ram-test/
```

In the storage server machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./DEBE/Prototype/bin
```

### Exp#3 (Multi-client uploads and downloads)

**Note that you need to deploy multiple clients in different machines and each client has a unique id**

- **step-1: preparation** (we use the case of **two** clients as an example)

In the client machine 1 (each client machine has a **unique** id):

```shell
# please set the config.json: "clientID_": 1
# prepare a 6GiB ramdisk in "~/ram-test/"
$ sudo mount -t tmpfs -o rw,size=6G tmpfs ~/ram-test/

# prepare the synthetic trace (put it in the memory)
$ cd ./DEBE/Prototype/bin
# generate a 2GiB file (with the random seed as 1): here use the **client ID** (1) as the seed
$ ./compressSYN -i 2048 -o ~/ram-test/test-2G-syn -s 1

$ cd ./DEBE/Prototype
$ bash ./script/exp3/exp3_pre.sh
```

In the client machine 2 (each client machine has a **unique** id):

```shell
# please set the config.json: "clientID_": 2
# prepare a 6GiB ramdisk in "~/ram-test/"
$ sudo mount -t tmpfs -o rw,size=6G tmpfs ~/ram-test/

# prepare the synthetic trace (put it in the memory)
$ cd ./DEBE/Prototype/bin
# generate a 2GiB file (with the random seed as 1): here use the **client ID** (2) as the seed
$ ./compressSYN -i 2048 -o ~/ram-test/test-2G-syn -s 2

$ cd ./DEBE/Prototype
$ bash ./script/exp3/exp3_pre.sh
```

In the storage server machine:

```shell
# store the containers in the memory
$ cd ./DEBE/Prototype
$ sudo mount -t tmpfs -o rw,size=22G tmpfs ./bin # create a large ramdisk here

$ cd ./DEBE/Prototype
$ bash ./script/exp3/exp3_pre.sh
```

- **step-2: evaluation**

Let those two client machines upload the file at the same time:

```shell
# you can use a script to send the command to all clients or use multiple terminals to send it
$ ./DEBEClient -t u -i ~/ram-test/test-2G-syn

# mannually check the largest time among all clients, e.g., 10.654321
# the aggreated upload speed: 2048 * 2 / 10.654321 = 384.444958 MiB/s

# you can use a script to send the command to all clients or use multiple terminals to send it
$ ./DEBEClient -t d -i ~/ram-test/test-2G-syn

# mannually check the largest time among all clients, e.g., 5.054321
# the aggreated upload speed: 2048 * 2 / 5.054321 = 810.395699 MiB/s
```

Reset the storage server machine (**you need to reset the storage server before each test**):

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Prototype
$ base recompile.sh
# clean all stored containers in ./bin
```

- **step-3: clean up**

In each client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ~/ram-test/
```

In the storage server machine:

```shell
# unmount the ramdisk
$ sudo umount -l ./DEBE/Prototype/bin
```

### Exp#4 (Impact of frequency distribution)

- **step-1: preparation**

In the client machine:

```shell
# since we use synthetic traces as the inputs
# please set the config.json: "chunkingType_": 2

$ cd ./DEBE/Prototype
$ bash ./script/exp4/exp4_pre.sh

# generate the trace used in this experiment
$ bash ./script/gen_zipf_trace.sh
# it would generate 4 trace files
syn_zipf_0_8 # zipf constant 0.8
syn_zipf_0_9 # zipf constant 0.9
syn_zipf_1_0 # zipf constant 1.0
syn_zipf_1_1 # zipf constant 1.1
```

In the storage server machine:

```shell
$ cd ./DEBE/Prototype
$ bash ./script/exp4/exp4_pre.sh
```

- **step-2: evaluation (use "zipf constant 0,8 and k = 512K" as an example)**

In the storage server machine:

```shell
# check the config.json, ensure topKParam_: 512
# start the storage server
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -m 4
...
...
2022-05-10 01:48:19 <DEBEServer>: waiting the request from the client.
```

In the client machine:

```shell
# check the config.json, ensure chunkingType_: 2 (to process the zipf trace file)
$ cd ./DEBE/Prototype/bin
$ ./DEBEClient -t u -i ../syn_zipf_0_8
```

In the storage server machine:

```shell
# check the result in the storage server machine
$ cd ./DEBE/Prototype/bin
$ cat server-log
logical data size (B), logical chunk num, unique data size (B), unique chunk num, compressed data size (B), total process time (s), enclave speed (MiB/s)
107374182400, 13107200, 21474836480, 2621440, 10795936893, 369.318348, 277.267568

# the last column is the enclave process speed: 277.267568 MiB/s
```

Note that you can vary `topKParam_` in `config.json` to change the parameter of the top-k index. Also, you can upload different trace files to test the case with different zipf constants (**you need to reset the storage server before each test**).

Reset the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Prototype
$ base recompile.sh
# clean all stored containers in ./bin
```

## Evaluation on Real-world Traces

### Exp#5 (Performance of deduplication approaches) && Exp#7 (Storage efficiency)

- **step-1: preparation**

In the client machine:

```shell
# prepare a 4GiB ramdisk in "~/ram-test/"
$ sudo mount -t tmpfs -o rw,size=4G tmpfs ~/ram-test/

# prepare the scripts for running traces (assume you have downloaded all traces)
$ cd ./DEBE/Prototype
# for LINUX: 
$ python3 ./script/trace_exp.py -i ~/LINUX_trace -o linux-upload.sh -t u -s 0 -f 1 -d ~/ram-test/

# for CONTAINER
$ python3 ./script/trace_exp.py -i ~/CONTAINER_trace -o container-upload.sh -t u -s 0 -f 1 -d ~/ram-test/

# for VM
$ python3 ./script/trace_exp.py -i ~/VM_fp/0330 -o 0330-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/VM_fp/0413 -o 0413-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/VM_fp/0427 -o 0427-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/VM_fp/0511 -o 0511-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ cat 0330-upload.sh 0413-upload.sh 0427-upload.sh 0511-upload.sh > vm-upload.sh

# for FSL
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-01-22 -o 01-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-02-22 -o 02-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-03-22 -o 03-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-04-22 -o 04-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-05-17 -o 05-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-06-17 -o 06-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ cat 01-upload.sh 02-upload.sh 03-upload.sh 04-upload.sh 05-upload.sh 06-upload.sh > fsl-upload.sh

# for MS
$ python3 ./script/trace_exp.py -i ~/MS_fp -o ms-upload.sh -t u -s 0 -f 1 -d ~/ram-test/

# config your config.json
# For LINUX and CONTAINER: chunkingType_: 1
# For FSL and VM: chunkingType_: 2
# For MS: chunkingType_: 3
# topKParam_: 512

$ cd ./DEBE/Prototype
$ bash ./script/exp5/exp5_pre.sh
```

In the storage server machine:

```shell
$ cd ./DEBE/Prototype
$ bash ./script/exp5/exp5_pre.sh
```

- **step-2: evaluation**

In the storage server machine:

```shell
# start the storage server
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -m 4
...
...
2022-05-10 01:48:19 <DEBEServer>: waiting the request from the client.
```

In the client machine:

```shell
# we use FSL as an example
$ cd ./DEBE/Prototype/bin
$ bash ../fsl-upload.sh
# it will upload the snapshot one by one
```

In the storage server machine:

```shell
# you can check the upload time and storage efficiecny after processing each sanpshot
$ cd ./DEBE/Prototype/bin
$ cat server-log
logical data size (B), logical chunk num, unique data size (B), unique chunk num, compressed data size (B), total process time (s), enclave speed (MiB/s)
10111200989, 1069738, 6925368727, 740314, 3113984243, 35.667100, 270.355389 
63201764429, 6274867, 41272613537, 4294080, 18546028255, 183.854602, 275.386671 
66138000660, 6606258, 43823485632, 4579799, 19694588600, 11.204024, 249.929229 
322101071959, 31643865, 131242506040, 13342502, 58940105236, 800.829790, 304.815593 
332093235812, 33428242, 138522339612, 14432069, 62256674516, 37.047536, 257.217377 
457572686583, 46789835, 215704979182, 22931150, 97000641365, 432.471394, 276.703919 
460433737695, 47125781, 216752705618, 23085156, 97475576565, 9.039478, 301.843863 
534330508793, 55419310, 261259176280, 28142448, 117501881155, 256.693972, 274.542686 
544454429833, 56490603, 261327994530, 28149723, 117532841049, 28.063472, 344.038809 
591571240127, 60995494, 266733529028, 28688308, 119959379825, 133.994917, 335.341764 
594507476358, 61326885, 266733529028, 28688308, 119959379825, 8.204528, 341.300935 
851701029157, 86531174, 267127264161, 28729444, 120136529219, 705.742404, 347.547326 
861714736269, 88449627, 267525216424, 29162874, 120367239071, 30.804356, 310.015093 
999156608268, 103299981, 283768724192, 30839574, 127666688366, 396.897543, 330.248412 
1002017659380, 103635927, 283768724192, 30839574, 127666688366, 7.939727, 343.652995 
1076314654402, 111967769, 284345329315, 30901136, 127925895854, 205.615136, 344.600779
....

# the last column (enclave speed) is the enclave computational speed (Exp5, Fig~5)
# the reducation ratio is computed as (Exp7, Fig~10):
# 	for deduplication-only (D): logical data size / unique data size
# 	for deduplication+compression: logical data size / compressed data size
# for the key metadata overhead (Exp4, Fig~11)
# 	DEBE: unique chunk num * 16 + 2 * 32
#	DaE: logical chunk num * 32
```

Note that you can vary the parameter of  `-m` of `./DEBEServer` to test with different approaches (see the README file in `./DEBE/Prototype`). (Since the `-m 1` (in-enclave index) approach is very **slow** and would take several days to process FSL, MS, and VM, we do not recommend to run in-enclave index in AE for those traces). Also, **you need to reset the storage server before each test**.

Reset the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Prototype
$ base recompile.sh
# clean all stored containers in ./bin
```

- **step-3: clean up**

In the client machine:

```shell
# unmount the ramdisk
$ sudo umount -l ~/ram-test/
```

### Exp#6 (Trace-driven upload and download)

- **step-1: preparation**

Prepare the trace scripts for DEBE (in the client machine):

```shell
# prepare the scripts for running traces (assume you have downloaded all traces)
$ cd ./DEBE/Prototype
# for LINUX: 
$ python3 ./script/trace_exp.py -i ~/LINUX_trace -o debe-linux-upload.sh -t u -s 0 -f 1 -d ~/ram-test/

# for CONTAINER
$ python3 ./script/trace_exp.py -i ~/CONTAINER_trace -o debe-container-upload.sh -t u -s 0 -f 1 -d ~/ram-test/

# for VM
$ python3 ./script/trace_exp.py -i ~/VM_fp/0330 -o 0330-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/VM_fp/0413 -o 0413-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/VM_fp/0427 -o 0427-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/VM_fp/0511 -o 0511-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ cat 0330-upload.sh 0413-upload.sh 0427-upload.sh 0511-upload.sh > debe-vm-upload.sh

# for FSL
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-01-22 -o 01-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-02-22 -o 02-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-03-22 -o 03-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-04-22 -o 04-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-05-17 -o 05-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-06-17 -o 06-upload.sh -t u -s 0 -f 1 -d ~/ram-test/
$ cat 01-upload.sh 02-upload.sh 03-upload.sh 04-upload.sh 05-upload.sh 06-upload.sh > debe-fsl-upload.sh

# for MS
$ python3 ./script/trace_exp.py -i ~/MS_fp -o debe-ms-upload.sh -t u -s 0 -f 1 -d ~/ram-test/

# also follow the same way to prepare the download scripts by change the "-t" parameter to "d"
# generate: debe-linux-download.sh, debe-container-download.sh, debe-vm-download.sh, debe-fsl-download.sh, and debe-ms-download.sh
```

Prepare the trace scripts for baseline approaches (in the client machine):

```shell
# prepare the scripts for running traces (assume you have downloaded all traces)
$ cd ./DEBE/Baseline
# for LINUX: 
$ python3 ./script/trace_exp.py -i ~/LINUX_trace -o base-linux-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1

# for CONTAINER
$ python3 ./script/trace_exp.py -i ~/CONTAINER_trace -o base-container-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1

# for VM
$ python3 ./script/trace_exp.py -i ~/VM_fp/0330 -o 0330-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/VM_fp/0413 -o 0413-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/VM_fp/0427 -o 0427-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/VM_fp/0511 -o 0511-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ cat 0330-upload.sh 0413-upload.sh 0427-upload.sh 0511-upload.sh > base-vm-upload.sh

# for FSL
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-01-22 -o 01-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-02-22 -o 02-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-03-22 -o 03-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-04-22 -o 04-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-05-17 -o 05-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ python3 ./script/trace_exp.py -i ~/FSL_fp/2013-06-17 -o 06-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1
$ cat 01-upload.sh 02-upload.sh 03-upload.sh 04-upload.sh 05-upload.sh 06-upload.sh > base-fsl-upload.sh

# for MS
$ python3 ./script/trace_exp.py -i ~/MS_fp -o base-ms-upload.sh -t u -s 0 -f 1 -d ~/ram-test/ -m 1

# also follow the same way to prepare the download scripts by change the "-t" parameter to "d"
# generate: base-linux-download.sh, base-container-download.sh, base-vm-download.sh, base-fsl-download.sh, and base-ms-download.sh
```

In the client machine:

```shell
# prepare a 4GiB ramdisk in "~/ram-test/"
$ sudo mount -t tmpfs -o rw,size=4G tmpfs ~/ram-test/

# config your config.json
# For LINUX and CONTAINER: chunkingType_: 1
# For FSL and VM: chunkingType_: 2
# For MS: chunkingType_: 3
# topKParam_: 512

# Note that for generating the trace scripts for the baseline appraches, you need to use the script/trace_exp.py in the ./DEBE/Baseline and follow the same way to prepare the scripts for baseline approaches (only add an extra parameter '-m' to choose the type of baseline approaches)
# can run "python3 script/trace_exp.py -h" for the detailed usage
$ cd ./DEBE/Prototype
$ bash ./script/exp6/exp6_pre.sh
$ cd ./DEBE/Baseline
$ bash ./recompile.sh
```

In the storage server machine:

```shell
$ cd ./DEBE/Prototype
$ bash ./script/exp6/exp6_pre.sh
```

- **step-2:  evaluate DEBE**

In the storage server machine:

```shell
# start the storage server
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -m 4
...
...
2022-05-10 01:48:19 <DEBEServer>: waiting the request from the client.
```

In the client machine:

```shell
# to test the DEBE end-to-end performance (use FSL as an example)
$ cd ./DEBE/Prototype/bin
# perform the upload test
$ bash ../debe-fsl-upload.sh

# when it finishes, you can check the result in the client-log
$ cat client-log
input file, opt, logical data size (B), logical chunk num, total time (s), speed (MiB/s)
fslhomes-user004-2013-01-22, upload, 10111200989, 1069738, 44.017753, 219.065991  
fslhomes-user007-2013-01-22, upload, 53090563440, 5205129, 215.607884, 234.829571
fslhomes-user008-2013-01-22, upload, 2936236231, 331391, 13.369500, 209.447854
fslhomes-user012-2013-01-22, upload, 255963071299, 25037607, 959.700679, 254.355772 
fslhomes-user013-2013-01-22, upload, 9992163853, 1784377, 43.100789, 221.092705
fslhomes-user015-2013-01-22, upload, 125479450771, 13361593, 536.573705, 223.019742 
fslhomes-user022-2013-01-22, upload, 2861051112, 335946, 10.761342, 253.547463
fslhomes-user028-2013-01-22, upload, 73896771098, 8293529, 316.358606, 222.764456
fslhomes-user004-2013-02-22, upload, 10123921040, 1071293, 34.046710, 283.578750
fslhomes-user007-2013-02-22, upload, 47116810294, 4504891, 162.179352, 277.064196
fslhomes-user008-2013-02-22, upload, 2936236231, 331391, 9.953647, 281.325335
fslhomes-user012-2013-02-22, upload, 257193552799, 25204289, 858.275169, 285.781174 
fslhomes-user013-2013-02-22, upload, 10013707112, 1918453, 38.826027, 245.964267
fslhomes-user015-2013-02-22, upload, 137441871999, 14850354, 480.608923, 272.726487 
fslhomes-user022-2013-02-22, upload, 2861051112, 335946, 10.271083, 265.649782
fslhomes-user028-2013-02-22, upload, 74296995022, 8331842, 250.481944, 282.875224
...
# the last column is the DEBE end-to-end upload performance (Exp6, Fig~9)

# perform the download test
# unmount the ramdisk, as we will let the client store data in the disk
$ sudo umount -l ~/ram-test/
$ bash ../debe-fsl-download.sh
# when it finishes, you can also check the result in the client-log
$ cat client-log
```

Reset the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Prototype
$ base recompile.sh
# clean all stored containers in ./bin
```

- **step-3: evaluate the baseline approaches**

In the storage server machine:

```shell
# start the storage server
$ cd ./DEBE/Baseline/bin
$ ./DAEServer
...
...
2022-05-10 02:16:17 <DAEServer>: waiting the request from the client.
```

In the client machine: 

```shell
# prepare a 4GiB ramdisk in "~/ram-test/" (use FSL as an example)
$ sudo mount -t tmpfs -o rw,size=4G tmpfs ~/ram-test/

$ cd ./DEBE/Baseline/bin
$ bash ../base-fsl-upload.sh # (see how to generate the scripts for baseline approaches above)
# when it finishes, you can also check the result in the client-log
$ cat client-log
input file, opt, logical data size (B), logical chunk num, total time (s), speed (MiB/s)
fslhomes-user004-2013-01-22, upload, 10111200989, 1069738, 53.759908, 179.367731
fslhomes-user007-2013-01-22, upload, 53090563440, 5205129, 282.012017, 179.535281
fslhomes-user008-2013-01-22, upload, 2936236231, 331391, 16.213474, 172.709012
fslhomes-user012-2013-01-22, upload, 255963071299, 25037607, 1354.500683, 180.218002
fslhomes-user013-2013-01-22, upload, 9992163853, 1784377, 54.580099, 174.592392
fslhomes-user015-2013-01-22, upload, 125479450771, 13361593, 702.693924, 170.296804
fslhomes-user022-2013-01-22, upload, 2861051112, 335946, 15.384485, 177.354716
fslhomes-user028-2013-01-22, upload, 73896771098, 8293529, 403.944453, 174.463227
fslhomes-user004-2013-02-22, upload, 10123921040, 1071293, 53.254670, 181.297217
fslhomes-user007-2013-02-22, upload, 47116810294, 4504891, 254.548447, 176.524714
fslhomes-user008-2013-02-22, upload, 2936236231, 331391, 15.472660, 180.978130
fslhomes-user012-2013-02-22, upload, 257193552799, 25204289, 1355.762604, 180.915807
fslhomes-user013-2013-02-22, upload, 10013707112, 1918453, 54.861478, 174.071418
fslhomes-user015-2013-02-22, upload, 137441871999, 14850354, 721.459267, 181.680088
fslhomes-user022-2013-02-22, upload, 2861051112, 335946, 15.331814, 177.964001
fslhomes-user028-2013-02-22, upload, 74296995022, 8331842, 396.407896, 178.742998
...
# the last column is the CE end-to-end upload performance (Exp6, Fig~9)

# perform the download test
# unmount the ramdisk, as we will let the client store data in the disk
$ sudo umount -l ~/ram-test/
$ bash ../base-fsl-download.sh
# when it finishes, you can also check the result in the client-log
$ cat client-log
```

 Also, **you need to reset the storage server before each test**.

Reset the storage server machine:

```shell
# press "ctrl + c" to stop the storage server
$ cd ./DEBE/Baseline
$ base recompile.sh
# clean all stored containers in ./bin
```

### Exp#8 (Security against frequency analysis)

We use the trace analysis tool (i.e., `./DEBE/Sim`) in this experiment. 

- **step-1: preparation**

```shell
# prepare the traces
$ cat ~/LINUX_fp/* > LINUX-ALL
$ cat ~/MS_fp/* > MS-ALL
$ cat ~/FSL_fp/*/* > FSL-ALL
$ cat ~/VM_fp/*/* > VM-ALL
$ cat ~/CONTAINER_fp/* > CONTAINER-ALL

# combine all snapshots of each trace into one single file
```

- **step-2: evaluation**

We use running LINUX trace with k = 512K as an example:

```shell
# we evaluate the frequency information leakage (i.e., KLD) in DEBE when k = 512K
$ cd ./DEBE/Sim/bin
$ ./KLDMain -i ~/LINUX-ALL -m 0 -k 512 -t 0
2022-05-05 15:35:39 <Sim>: --------config--------
2022-05-05 15:35:39 <Sim>: input file path: /home/zryang/LINUX-ALL
2022-05-05 15:35:39 <Sim>: threshold: 512 K
2022-05-05 15:35:39 <Sim>: ----------------------
Database: using LevelDB.
Database: using LevelDB.
2022-05-05 15:35:39 <FreqIndex>: init the FreqIndex.
2022-05-05 15:35:39 <FreqIndex>: start to process /home/zryang/LINUX-ALL
2022-05-05 15:36:32 <Sim>: total running time: 52.687174
========FreqIndex Info========
Original KLD (CE): 1.048819
Cipher KLD (DEBE): 0.088357
==============================
# the KLD of DEBE with k=512K is 0.088357
# the KLD of CE is 1.048819
```

Note that you can vary the parameters of  `-m`, `-k`, and `-t` to test different traces with different approaches (see the README file in `./DEBE/Sim`).
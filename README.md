# DEBE

## Introduction
DEBE is a shielded DbE-based deduplicated storage system that protects deduplication via Intel SGX. This repo contains the implementation of DEBE prototype, baseline approaches, and a trace analysis tool used in our USENIX ATC 2022 paper.

- `./Prototype`: includes the implementation of DEBE prototype.
- `./Baseline`: includes the implementation of all baseline approaches (e.g., DupLESS, TED, CE, and Plain).
- `./Sim`: includes a trace analysis tool to measure the frequency leakage of CE, TED, and DEBE.

Note that each folder has a separate README file to introduce the build instructions and usage.

## Publication
- Zuoru Yang, Jingwei Li, and Patrick P. C. Lee. [Secure and Lightweight Deduplicated Storage via Shielded Deduplication-Before-Encryption.](http://www.cse.cuhk.edu.hk/~pclee/www/pubs/atc22.pdf). USENIX ATC 2022.

## Dependencies

- Basic packages: clang, llvm, cmake, libboost-all-dev, libleveldb-dev, jemalloc, liblz4-dev, jq, golang-go, and openssl 

The packages above can be directly installed via `apt-get`:
```shell
sudo apt-get install llvm clang libboost-all-dev libleveldb-dev libjemalloc-dev liblz4-dev libssl-dev jq golang-go
```
Note that we require the version of OpenSSL should be at least **1.1.0**. If the default version of OpenSSL from `apt-get` is older than 1.1.0, please install OpenSSL manually from this [link](https://www.openssl.org/source/).

- SGX-related packages: 
  - Intel® Software Guard Extensions (Intel® SGX) driver [link](https://github.com/intel/linux-sgx-driver)
  - Intel® SGX SDK [link ](https://01.org/intel-software-guard-extensions/downloads)
  - Intel® Software Guard Extensions SSL [link](https://github.com/intel/intel-sgx-ssl#intel-software-guard-extensions-ssl)

The above packages can be installed from the corresponding links. In our paper, DEBE is tested with Intel SGX SDK Linux 2.7 and Intel SGX SSL-1.1.1 in Ubuntu 16.04 LTS. If your OS version is higher than Ubuntu 16.04 LTS (e.g., Ubuntu 20.04 LTS), you might **not** be able to install the packages with the same versions as in our paper. Nevertheless, we expect that the impact of using the packages with different versions would be limited and our prototype can still run correctly.

## Build & Usage

Please refer to the README files in `./Prototype`, `./Baseline`, and `./Sim` for the building instructions and usage of each component.

## Traces

We use five real-world datasets of backup workloads in our paper. For DOCKER and LINUX, they contain the real content and can be downloaded via scripts in `./Prototype/script/trace`. To analyze the frequency leakage in Exp#8, we also calculate the fingerprint of each chunk in both traces and store their fingerprint lists on a public site for downloads (see below). For VM, FSL, and MS, they only contain chunk fingerprints and we generate their compressible chunk contents by using [LZ data generator](https://github.com/jibsen/lzdatagen) (we have also included the LZ data generator in our prototype). 

Here, we introduce how to collect each trace as follows.

- DOCKER
```shell
# download the real content of the DOCKER trace
cd ./Prototype
python3 script/trace/get_container.py # download trace from DockerHub, it will store the tar file of each version in "~/CONTAINER_trace/"

# download the chunk fingerprint list of each snapshot in the DOCKER trace (for Exp#8)
# link: https://drive.google.com/file/d/1IPOld2XaWwkk1iYZU3t5TSzsOQWdJJoZ/view?usp=sharing
# the name of the file: CONTAINER_fp.zip
gdown https://drive.google.com/uc?id=1IPOld2XaWwkk1iYZU3t5TSzsOQWdJJoZ # download via gdown (can be installed from pip3)
unzip ./CONTAINER_fp.zip # uncompress the file, the chunk fingerprint lists are in the folder "CONTAINER_fp"
```

- LINUX
```shell
# install "mtar" to generate the deduplication-aware tar, from the link: https://github.com/xinglin/mtar
# ensure "/usr/local/bin/tar" exists
cd ./Prototype
python3 script/trace/get_linux.py # download the trace from Github, it will store the mtar file of each version in "~/LINUX_trace"

# download the chunk fingerprint list of each snapshot in the LINUX trace (for Exp#8)
# link: https://drive.google.com/file/d/1-Y_l3426Q5u8yzhmSy9qRwx-MVHhtAYl/view?usp=sharing
# the name of the file: LINUX_fp.zip
cd ~/
gdown https://drive.google.com/uc?id=1-Y_l3426Q5u8yzhmSy9qRwx-MVHhtAYl
unzip ./LINUX_fp.zip # uncompress the file, the chunk fingerprint lists are in the folder "LINUX_fp"
```

- VM
```shell
# download the chunk fingerprint list of each snapshot in the VM trace 
# link: https://drive.google.com/file/d/1WAAjBBqWG2MG3IONyOIu2j_NWp-rVqIf/view?usp=sharing
# the name of the file: VM_fp.zip
cd ~/
gdown https://drive.google.com/uc?id=1WAAjBBqWG2MG3IONyOIu2j_NWp-rVqIf
unzip ./VM_fp.zip # uncompress the file, the chunk fingerprint lists are in the folder "VM_fp"
```

- FSL
```shell
# download the chunk fingerprint list of each snapshot in the FSL trace 
# link: https://drive.google.com/file/d/1RdSaBoIKf-PQOM79T1lzMDzg88sSwSII/view?usp=sharing
# the name of the file: FSL_fp.zip
cd ~/
gdown https://drive.google.com/uc?id=1RdSaBoIKf-PQOM79T1lzMDzg88sSwSII
unzip ./FSL_fp.zip # uncompress the file, the chunk fingerprint lists are in the folder "FSL_fp"
```

- MS
```shell
# download the chunk fingerprint list of each snapshot in the MS trace 
# link: https://drive.google.com/file/d/1lYgHPxyuuMtVF4pyBQS6pK1kJvxde5sZ/view?usp=sharing
# the name of the file: MS_fp.zip
cd ~/
gdown https://drive.google.com/uc?id=1lYgHPxyuuMtVF4pyBQS6pK1kJvxde5sZ
unzip ./MS_fp.zip # uncompress the file, the chunk fingerprint lists are in the folder "MS_fp"
```

## Getting Started Instructions

Here we provide some instructions to quickly check the effectiveness of each component.

- Preparation 

Please ensure that you have successfully compiled three components based on their README files in each folder and prepare two machines (a client and a storage server) connected by the network.

In the client machine, you need to configure `storageServerIp_` to the storage server machine IP in `config.json`. (e.g., "storageServerIp_": "192.168.10.64")

**Not that we recommend to edit the `config.json` in `./DEBE/Prototype` (or `./DEBE/Baseline` for baseline approaches)  and run `recompile.sh` to enforce the configuration (it will automatically copy the `config.json` to `./DEBE/Prototype/bin` or `./DEBE/Baseline/bin`).** 

- DEBE

We verify the DEBE effectiveness via uploading/downloading a 2GiB file (as Exp#1).

In the storage server machine:

```shell
# start the storage server
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -m 4
Database: using In-Memory Index.
InMemoryDatabase: cannot open the db file.
InMemoryDatabase: db file file is not exists, create a new one.
InMemoryDatabase: loaded index size: 0
2022-05-06 22:24:26 <SSLConnection>: init the connection to port 16666
2022-05-06 22:24:26 <DEBEServer>: SGX is enable.
2022-05-06 22:24:27 <DEBEServer>: create the enclave successfully.
2022-05-06 22:24:27 <OCall>: print the file name: enclave-key
2022-05-06 22:24:27 <OCall>: sealed file does not exist.
2022-05-06 22:24:27 <DataWriter>: init the DataWriter.
**Enclave**: <StorageCore>: init the StorageCore.
**Enclave**: <EnclaveBase>: init the EnclaveBase.
2022-05-06 22:24:27 <OCall>: print the file name: cm-sketch
2022-05-06 22:24:27 <OCall>: sealed file does not exist.
**Enclave**: <EcallFreqIndex>: do not need to load the index.
**Enclave**: <EcallFreqIndex>: init the EcallFreqIndex.
2022-05-06 22:24:27 <OCall>: print the file name: enclave-index-info
2022-05-06 22:24:27 <OCall>: sealed file does not exist.
2022-05-06 22:24:27 <EnclaveIndex>: init the EnclaveIndex.
2022-05-06 22:24:27 <DataReceiver>: init the DataReceiver.
2022-05-06 22:24:27 <AbsRecvDecoder>: init the AbsRecvDecoder.
**Enclave**: <EcallRecvDecoder>: init the RecvDecoder.
2022-05-06 22:24:27 <EnclaveRecvDecoder>: init the EnclaveRecvDecoder.
2022-05-06 22:24:27 <RAUtil>: init the RAUtil.
2022-05-06 22:24:27 <ServerOptThread>: init the ServerOptThread.
2022-05-06 22:24:27 <DEBEServer>: waiting the request from the client.
```

In the client machine:

```shell
$ cd ./DEBE/Prototype/bin
# prepare a test file
$ ./compressSYN -i 2048 -o ~/test-2G-syn -s 1

# upload the file to the storage server
$ ./DEBEClient -t u -i ~/test-2G-syn

# download the file from the storage server
$ ./DEBEClient -t d -i ~/test-2G-syn

# you can check the correctness of the file via md5sum
$ md5sum ~/test-2G-syn*
cd8450d99771a94cf4f692a0f9bf09b7  test-2G-syn
cd8450d99771a94cf4f692a0f9bf09b7  test-2G-syn-d
# same means the download file is correct

# upload the same file again to test max performance
$ ./DEBEClient -t u -i ~/test-2G-syn

# check the end-to-end performance (the last column)
$ cat client-log
input file, opt, logical data size (B), logical chunk num, total time (s), speed (MiB/s)
test-2G-syn, upload, 2147483648, 226856, 9.725359, 210.583485
test-2G-syn, download, 2147483648, 226856, 13.700910, 149.479122
test-2G-syn, upload, 2147483648, 226856, 6.952322, 294.577840
# upload unique data speed: 210.6 MiB/s
# download data speed: 149.5 MiB/s
# upload duplicate data speed: 294.6 MiB/s
```

In the storage server machine:

```shell
# check the enclave computational speed (the last column)
$ cd ./DEBE/Prototype/bin
$ cat server-log
logical data size (B), logical chunk num, unique data size (B), unique chunk num, compressed data size (B), total process time (s), enclave speed (MiB/s)             
2147483648, 226856, 2147483648, 226856, 1104978793, 8.339103, 245.589963
4294967296, 453712, 2147483648, 226856, 1104978793, 5.736177, 357.032218
# process unique data speed: 245.6 MiB/s
# process duplicate data speed: 357.0 MiB/s

# reset the server and clear the data
# Please first press ctrl+c to stop the storage server!
$ cd ./DEBE/Prototype
$ bash recompile.sh
# clear all store data and re-enforce the configuration
```

- Baseline

We use CE to verify the effectiveness of the baseline approach (as Exp#1).

In the storage server machine:

```shell
$ cd ./DEBE/Baseline/bin
$ ./DAEServer
Database: using In-Memory Index.
InMemoryDatabase: cannot open the db file.
InMemoryDatabase: db file file is not exists, create a new one.
InMemoryDatabase: loaded index size: 0
2022-05-06 23:03:03 <SSLConnection>: init the connection to port 16666
2022-05-06 23:03:03 <DataWriter>: init the DataWriter.
2022-05-06 23:03:03 <StorageCore>: Init the StorageCore
2022-05-06 23:03:03 <DedupIndex>: init the PlainIndex.
2022-05-06 23:03:03 <DataReceiver>: init the DataReceiver.
2022-05-06 23:03:03 <AbsRecvDecoder>: init the AbsRecvDecoder.
2022-05-06 23:03:03 <RecvDecoder>: init the RecvDecoder.
2022-05-06 23:03:03 <ServerOptThread>: init the ServerOptThread.
2022-05-06 23:03:03 <DAEServer>: waiting the request from the client.
```

In the client machine:

```shell
$ cd ./DEBE/Baseline/bin

# upload the file to the storage server, use the same file above
$ ./DAEClient -t u -i ~/test-2G-syn -m 1

# download the file from the storage server
$ ./DAEClient -t d -i ~/test-2G-syn -m 1

# you can check the correctness of the file via md5sum
$ md5sum ~/test-2G-syn*
cd8450d99771a94cf4f692a0f9bf09b7  test-2G-syn
cd8450d99771a94cf4f692a0f9bf09b7  test-2G-syn-d
# same means the download file is correct

# upload the same file again to test max performance
$ ./DAEClient -t u -i ~/test-2G-syn -m 1

# check the end-to-end performance (the last column)
$ cat client-log
input file, opt, logical data size (B), logical chunk num, total time (s), speed (MiB/s)
test-2G-syn, upload, 2147483648, 226856, 11.474590, 178.481323
test-2G-syn, download, 2147483648, 226856, 13.815944, 148.234533
test-2G-syn, upload, 2147483648, 226856, 11.368715, 180.143490
# upload unique data speed: 178.5 MiB/s
# download data speed: 148.2 MiB/s
# upload duplicate data speed: 180.1 MiB/s (should be lower than DEBE)
```

In the storage server machine:

```shell
# reset the server and clear the data
# please first press ctrl+c to stop the storage server!
$ cd ./DEBE/Baseline
$ bash recompile.sh
# clear all store data and re-enforce the configuration
```

- Sim

We use LINUX trace to verify the effectiveness of the trace analysis tool (as Exp#8).

```shell
# combine all fingerprint lists in LINUX trace into a single file, assume the fingerprint lists are in the ~/LINUX_fp
$ cat ~/LINUX_fp/* > ~/LINUX-ALL # the final input trace file is "~/LINUX-ALL"

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
```

## Detailed Instructions

Please refer to the `ae_instruction.md` in `./Prototype` for detailed instructions to reproduce the experiments in our paper.

## Question

If you have any questions, please feel free to contact `zryang@cse.cuhk.edu.hk`

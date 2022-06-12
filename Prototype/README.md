# DEBE: Prototype
The prototype of DEBE includes the client, the storage server, and the synthetic trace generator for evaluation.
## Build
To compile the whole project:

```shell
$ cd ./DEBE/Prototype
$ bash setup.sh # for the first build
```

Note:

1. if your compiler outputs "cannot find **-lsgx_pthread**" (for some old version SGX SDKs), please remove the `-lsgx_pthread` in `./DEBE/cmake/FindSGX.cmake`.
2. If it cannot find **sgx_capable**, please copy this lib from the  SGX SDK:

```shell
# copy the to the "/usr/lib/x86_64-linux-gnu/"
$ sudo cp /opt/intel/sgxsdk/lib64/libsgx_capable.a /opt/intel/sgxsdk/lib64/libsgx_capable.so /usr/lib/x86_64-linux-gnu/
```

If the compilation is successful, the executable file is the `bin` folder:

```shell
# currently in the ./DEBE/Prototype
$ ls ./bin
compressSYN*  config.json  Containers/  DEBEClient*  DEBEServer*  Recipes/
```

`config.json`: the configuration file

`Containers`: the folder to store the container files

`Recipes`: the folder to store the file recipes

`DEBEClient`: the DEBE client

`DEBEServer`: the DEBE storage server

`compressSYN`: the synthetic trace generator to generate compressible traces

To re-compile the project and clean store data in the `bin` folder:

```shell
$ cd ./DEBE/Prototype
$ base recompile.sh # clean all store data
```

Note that `recompile.sh` will copy `./DEBE/Prototype/config.json` to `./DEBE/Prototype/bin` for re-configuration. Please ensure that `config.json` in `./DEBE/Prototype/bin` is correct in your configuration (You can edit the `./DEBE/Prototype/config.json` to set up your configuration and run `recompile.sh` to validate your configuration). 

## Usage

- Configuration: you can use `config.json` to configure the system.

```json
{   
    "ChunkerConfig":{
        "chunkingType_": 1, // chunking type: 1: FastCDC, 2: FSL,VM 3: MS (need to modify)
        "maxChunkSize_": 16384, // max chunk size
        "minChunkSize_": 4096, // avg chunk size
        "avgChunkSize_": 8192, // min chunk size
        "slidingWinSize_": 128, // chunking sliding window size
        "readSize_": 128 // read data buffer size
    },
    "StorageCore": {
        "recipeRootPath_": "Recipes/", // the recipe path
        "containerRootPath_": "Containers/", // the container path
        "fp2ChunkDBName_": "db1", // the name of the index file
        "topKParam_": 512 // the size of top-k index, unit (K, 1024)
    },
    "RestoreWriter": {
        "readCacheSize_": 64 // the restore container cache size
    },
    "DataSender": {
        "storageServerIp_": "127.0.0.1", // the storage server ip (need to modify)
        "storageServerPort_": 16666, // the storage server port (need to modify)
        "clientID_": 1, // the id of the client (can be modify)
        "localSecret_": "12345", // the client master key
        "sendChunkBatchSize_": 128, // the batch size of sending chunks
        "sendRecipeBatchSize_": 1024, // the batch size of sending key recipes
        "spid_": "259A7E2BC521D75621AEA63669BEA34D", // remote attestation setting
        "quoteType_": 0, // remote attestation setting
        "iasServerType_": 0, // remote attestation setting
        "iasPrimaryKey_": "fee17e94cd834ec7a3ed4e72bf04f795", // remote attestation setting
        "iasSecKey_": "0223f86f98154b6b9316054658eda2d3", // remote attestation setting
        "iasVersion_": 4 // remote attestation setting
    }
}
```

Note that you need to modify `storageServerIp_`, and `storageServerPort_` according to the machines that run the storage server.  

If you use **FSL** and **VM** traces, please set `chunkingType_` as 2; If you use **MS** trace, please set `chunkingType_` as 3; otherwise please set `chunkingType_` as 1.

- Client usage: 

check the command specification:

```shell
$ cd ./DEBE/Prototype/bin
$ ./DEBEClient -h
./DEBEClient -t [u/d/a] -i [inputFile path].
-t: operation ([u/d/a]):
        u: upload
        d: download
        a: remote attestation
```

`-t`: operation type, upload/download/remote attestation

`-i`: input file path

After each run, the client will record the running result in `client-log` in the `bin` folder.

- Storage server usage:

check the command specification:

```shell
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -h
./DEBEServer -m [IndexType]
-m: index type ([IndexType]):
        0: Out-Enclave Index
        1: In-Enclave Index
        2: Similarity-based Index
        3: Locality-based Index
        4: Freq-based Index
```

`-m`: the index type: `Out-Enclave`, `In-Enclave`, `Similarity-based`, `Locality-based`, and `Freq-based (final design of DEBE)` (see the Exp#5 and Exp#7 in our paper). 

Note that you can use "ctrl + c" to close the storage server when it is idle

- Synthetic trace generator usage

```shell
$ cd ./DEBE/Prototype/bin
$ ./compressSYN -h
./compressSYN -i [expected trace size (MiB)] -o [output trace file] -s [seed]
```

`-i`: expected output synthetic trace size

`-o`: the output trace file name

`-s`: the random seed used to generate the data content

Note that this generator would generate the trace with compression ratio as 2 by default.

## Example

Suppose we deploy the client and the storage server in two different machines (`config.json` is correctly configured). 

step-1: start the storage server in the storage server machine

```shell
$ cd ./DEBE/Prototype/bin
$ ./DEBEServer -m 4
Database: using In-Memory Index.
InMemoryDatabase: cannot open the db file.
InMemoryDatabase: db file file is not exists, create a new one.
InMemoryDatabase: loaded index size: 0
2022-05-06 21:02:09 <SSLConnection>: init the connection to port 16666
2022-05-06 21:02:09 <DEBEServer>: create the enclave successfully.
2022-05-06 21:02:09 <OCall>: print the file name: enclave-key
2022-05-06 21:02:09 <OCall>: sealed file does not exist.
2022-05-06 21:02:09 <DataWriter>: init the DataWriter.
**Enclave**: <StorageCore>: init the StorageCore.
**Enclave**: <EnclaveBase>: init the EnclaveBase.
2022-05-06 21:02:09 <OCall>: print the file name: cm-sketch
2022-05-06 21:02:09 <OCall>: sealed file does not exist.
**Enclave**: <EcallFreqIndex>: do not need to load the index.
**Enclave**: <EcallFreqIndex>: init the EcallFreqIndex.
2022-05-06 21:02:09 <OCall>: print the file name: enclave-index-info
2022-05-06 21:02:09 <OCall>: sealed file does not exist.
2022-05-06 21:02:09 <EnclaveIndex>: init the EnclaveIndex.
2022-05-06 21:02:09 <DataReceiver>: init the DataReceiver.
2022-05-06 21:02:09 <AbsRecvDecoder>: init the AbsRecvDecoder.
**Enclave**: <EcallRecvDecoder>: init the RecvDecoder.
2022-05-06 21:02:09 <EnclaveRecvDecoder>: init the EnclaveRecvDecoder.
2022-05-06 21:02:09 <RAUtil>: init the RAUtil.
2022-05-06 21:02:09 <ServerOptThread>: init the ServerOptThread.
2022-05-06 21:02:09 <DEBEServer>: waiting the request from the client.
```

step-2: start the client in the client machine

```shell
$ cd ./DEBE/Prototype/bin

# suppose we generate a 16MiB compressible file "test-16M"
$ ./compressSYN -i 16 -o test-16M -s 1
expected trace size (MiB): 16
output trace file: test-16M
random seed: 1
compressed size (MiB): 7.985490
total process size (MiB): 16

# suppose we upload "test-16M" to the storage server
$ ./DEBEClient -t u -i test-16M

# suppose we download "test-16M" from the storage server, it will be stored as "test-16M-d" in the same folder
$ ./DEBEClient -t d -i test-16M

# you can the correctness of the download file via using the checksun
$ md5sum test-16M*
d5287b1d17c09fc03a18c64c8281df1b  test-16M
d5287b1d17c09fc03a18c64c8281df1b  test-16M-d
# same chunksums mean they are identical
```

You can check the client log in `client-log` in the client machine (including the **input file name**, the **operation type**, the **logical data size**, the **number of chunks**, the **total running time**, and the **throughput**)

```shell
input file, opt, logical data size (B), logical chunk num, total time (s)total time (s), speed (MiB/s)
test-16M, upload, 16777216, 1742, 0.211287, 75.726382
test-16M, download, 16777216, 1742, 0.229753, 69.640005
```

The last column is the throughput (MiB/s).

You can also check the server log in `server-log` in the server machine (including the **accumulated logical data size**, the **accumulated number of chunks**, the **accumulated unique data size**, the **accumulated number of unique chunks**, the **accumulated compressed data size**, the **total enclave running time** of processing the input request)

```shell
logical data size (B), logical chunk num, unique data size (B), unique chunk num, compressed data size (B), total process time (s), enclave speed (MiB/s)             
16777216, 1742, 16777216, 1742, 8611630, 0.130629, 122.484288
```

- clean up

You can use "ctrl + c" to stop the server in the server machine:

```shell
^C2022-05-06 21:26:09 <DEBEServer>: terminate the server with ctrl+c interrupt
========DataWriter Info========
writer container num: 3
===============================
========EnclaveIndex Info========
recv data size: 16784184
recv batch num: 14
=================================
========DataReceiver Info========
total receive batch num: 14
total receive recipe end num: 1
=================================
========EnclaveRecvDecoder Info========
read container from file num: 3
=======================================
**Enclave**: <EcallFreqIndex>: ========EcallFreqIndex Info========
**Enclave**: <EcallFreqIndex>: logical chunk num: 1742
**Enclave**: <EcallFreqIndex>: logical data size: 16777216
**Enclave**: <EcallFreqIndex>: unique chunk num: 1742
**Enclave**: <EcallFreqIndex>: unique data size: 16777216
**Enclave**: <EcallFreqIndex>: compressed data size: 8611630
**Enclave**: <EcallFreqIndex>: ===================================
**Enclave**: <StorageCore>: ========StorageCore Info========
**Enclave**: <StorageCore>: write the data size: 8611630
**Enclave**: <StorageCore>: write chunk num: 1742
**Enclave**: <StorageCore>: ================================
========ServerOptThread Info========
total recv upload requests: 1
total recv download requests: 1
====================================
2022-05-06 21:26:09 <DEBEServer>: clear all server thread the object.
2022-05-06 21:26:09 <DEBEServer>: close all DBs and network connection.
```
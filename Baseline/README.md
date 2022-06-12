# DEBE: Baseline
Several baseline approaches to realize secure deduplicated storage. We compare DEBE with those baseline approaches (see Exp#1 and Exp#6 in our paper).
## Build
To compile the whole project:

```shell
$ cd ./DEBE/Baseline
$ bash setup.sh # for the first build
```

If the compilation is successful, the executable file is the `bin` folder:

```shell
# currently in the ./DEBE/Baseline
$ ls ./bin
config.json  Containers/  DAEClient*  DAEServer*  KeyManager*  Recipes/
```

`config.json`: the configuration file

`Containers`: the folder to store the container files

`Recipes`: the folder to store the file recipes and the key recipes

`DAEClient`: the baseline client

`DAEServer`: the baseline storage server

`KeyManager`: the key manager for key generation (in DupLESS and TED)

To re-compile the project and clean store data in the `bin` folder:

```shell
$ cd ./DEBE/Baseline
$ base recompile.sh # clean all store data
```

Note that `recompile.sh` will copy `./DEBE/Baseline/config.json` to `./DEBE/Baseline/bin` for re-configuration. Please ensure that `config.json` in `./DEBE/Baseline/bin` is correct in your configuration. 

## Usage

- Configuration: you can use `config.json` to configure the system.

```json
{
    "ChunkerConfig": {
        "chunkingType_": 1, // chunking type: 1: FastCDC, 2: FSL,VM 3: MS (need to modify)
        "maxChunkSize_": 16384, // max chunk size
        "avgChunkSize_": 8192, // avg chunk size
        "minChunkSize_": 4096, // min chunk size
        "slidingWinSize_": 128, // chunking sliding window size
        "readSize_": 128 // read data buffer size
    },
    "StorageCore": {
        "recipeRootPath_": "Recipes/", // the recipe path
        "containerRootPath_": "Containers/", // the container path
        "fp2ChunkDBName_": "db1" // the name of the index file
    },
    "RestoreWriter": {
        "readCacheSize_": 64 // the restore container cache size
    },
    "KeyServer": {
        "keyServerIp_": "127.0.0.1", // the key manager ip (need to modify)
        "keyServerPort_": 16667 // the key manager port (need to modify)
    },
    "DataSender": {
        "storageServerIp_": "127.0.0.1", // the storage server ip (need to modify)
        "storageServerPort_": 16666, // the storage server port (need to modify)
        "clientID_": 1, // the id of the client (can be modify)
        "localSecret_": "12345", // the client master key
        "sendChunkBatchSize_": 512, // the batch size of sending chunks
        "sendRecipeBatchSize_": 1024 // the batch size of sending key recipes
    }
}
```

Note that you need to modify `keyServerIp_`, `keyServerPort_`, `storageServerIp_`, and `storageServerPort_` according to the machines that run the key manager and the storage server.  

If you use **FSL** and **VM** traces, please set `chunkingType_` as 2; If you use **MS** trace, please set `chunkingType_` as 3; otherwise please set `chunkingType_` as 1.

- Client usage: 

check the command specification:

```shell
$ cd ./DEBE/Baseline/bin
$ ./DAEClient -h
./DAEClient -t [u/d] -i [inputFile path] -m [DaEType].
-t: operation ([u/d]):
        u: upload
        d: download
-m: DaE type ([DaEType]):
        0: Plain
        1: CE-DaE
        2: TED-DaE
        3: DupLESS-DaE
```

`-t`: operation type, upload/download

`-i`: input file path

`-m`: baseline method type: `Plain`, `CE`, `TED`, and `DupLESS`. Note that both `TED` and `DupLESS` need to deploy **a key manager** for key generation

After each run the client will record the running result in `client-log` in the `bin` folder.

- Key Manager usage: (in `TED` and `DupLESS`)

check the command specification:

```shell
$ cd ./DEBE/Baseline/bin
$ ./KeyManager -h
./KeyManager -m [type].
-m: method type ([type]):
        2: TED
        3: DupLESS
```

`-m`: key manager type (`TED` or `DupLESS`)

Note that you can use "ctrl + c" to close the key manager when it is idle

Note that only `TED` and `DupLESS` need to deploy a key manager in our evaluation.

- Storage server usage:

```shell
$ cd ./DEBE/Baseline/bin
$ ./DAEServer
```

Note that you can use "ctrl + c" to close the storage server when it is idle

## Example

Suppose we use `DupLESS` and deploy the client, the key manager, and the storage server in three different machines (`config.json` is correctly configured). 

step-1: start the key manager in the key manager machine

```shell
$ cd ./DEBE/Baseline/bin
$ ./KeyManager -m 3
2022-05-05 22:23:55 <SSLConnection>: init the connection to port 16667
2022-05-05 22:23:55 <DupLESSKM>: init the DupLESS-KeyManager.
2022-05-05 22:23:55 <KeyManager>: waiting the request from the client.
```

step-2: start the storage server in the storage server machine

```shell
$ cd ./DEBE/Baseline/bin
$ ./DAEServer
Database: using In-Memory Index.
InMemoryDatabase: cannot open the db file.
InMemoryDatabase: db file file is not exists, create a new one.
InMemoryDatabase: loaded index size: 0
2022-05-05 23:09:33 <SSLConnection>: init the connection to port 16666
2022-05-05 23:09:33 <DataWriter>: init the DataWriter.
2022-05-05 23:09:33 <StorageCore>: Init the StorageCore
2022-05-05 23:09:33 <DedupIndex>: init the PlainIndex.
2022-05-05 23:09:33 <DataReceiver>: init the DataReceiver.
2022-05-05 23:09:33 <AbsRecvDecoder>: init the AbsRecvDecoder.
2022-05-05 23:09:33 <RecvDecoder>: init the RecvDecoder.
2022-05-05 23:09:33 <ServerOptThread>: init the ServerOptThread.
2022-05-05 23:09:33 <DAEServer>: waiting the request from the client.
```

step-3: start the client in the client machine

```shell
$ cd ./DEBE/Baseline/bin

# suppose we upload "config.json" to the storage server
$ ./DAEClient -t u -i config.json -m 3

# suppose we download "config.json" from the storage server, it will be stored as "config.json-d" in the same folder
$ ./DAEClient -t d -i config.json -m 3

# you can the correctness of the download file via using the checksun
$ md5sum config.json*
d4d5d587923cc8f5730a6d3e9ed14536  config.json
d4d5d587923cc8f5730a6d3e9ed14536  config.json-d
# same chunksums mean they are identical
```

You can check the client log in `client-log` in the client machine (including the **input file name**, the **operation type**, the **logical data size**, the **number of chunks**, the **total running time**, and the **throughput**)

```shell
input file, opt, logical data size (B), logical chunk num, total time (s), speed (MiB/s)
config.json, upload, 746, 1, 0.066074, 0.010767
config.json, download, 746, 1, 0.021229, 0.033513
```

The last column is the throughput.

You can also check the server log in `server-log` in the server machine (including the **accumulated logical data size**, the **accumulated number of chunks**, the **accumulated unique data size**, the **accumulated number of unique chunks**, the **accumulated compressed data size**, the **total running time** of this request, and the **throughput**)

```shell
logical data size (B), logical chunk num, unique data size (B), unique chunk num, compressed data size (B), total time (s)
746, 1, 746, 1, 746, 0.000186
```

- clean up

You can use "ctrl + c" to stop the server in the server machine:

```shell
^C2022-05-05 23:40:35 <DAEServer>: terminate the server with ctrl+c interrupt
========DataWriter Info========
writer container num: 1
===============================
========StorageCore Info========
write the data size: 746
write chunk num: 1
================================
========PlainIndex Info========
recv data size: 750
recv batch num: 1
logical chunk num: 1
logical data size: 746
unique chunk num: 1
unique data size: 746
compressed data size: 746
===============================
========DataReceiver Info========
total recv batch num: 1
total recv recipe end num: 1
=================================
========RecvDecoder Info========
read container from file num: 0
=======================================
========ServerOptThread Info========
total recv upload requests: 1
total recv download requests: 0
====================================
2022-05-05 23:40:35 <DAEServer>: clear all server thread the object.
2022-05-05 23:40:35 <DAEServer>: close all DBs and network connection.
```
# DEBE: Sim

A trace analysis tool to measure the frequency leakage (e.g., KLD) of DEBE and TED in different traces (see Exp#8 in our paper).

## Build

To compile the trace analysis tool:

```shell
$ cd ./DEBE/Sim
$ bash setup.sh # for the first build
```

If the compilation is successful, the executable file is in the `bin` folder:

```shell
# currently in the ./DEBE/Sim
$ ls ./bin
KLDMain
```

To re-compile the tool and clean temporary files in the `bin` folder:

```shell
$ cd ./DEBE/Sim
$ bash recompile.sh # re-compile the project and clean the files in 'bin'
```

## Usage

check the command specification:

```shell
$ cd ./DEBE/Sim/bin
$ ./KLDMain -h
./KLDMain -i [input file] -m [method] -k [top-k threshold (K)] -t [trace type]
-m: method:
        0: DEBE
        1: TED
-t: trace type:
        0: DOCKER, LINUX, VM, and FSL
        1: MS
```

`-i`: the path of a chunk fingerprint list

`-m` : using DEBE or TED to process the chunk fingerprint list

`-k`: the number of the top-k (unit: **K**, i.e., **1024**)

`-t`: the trace type, if using MS trace, the type is `1`, otherwise the trace type is `0`


## Example

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

It shows that the KLD of DEBE is around 0.09 and the KLD of CE is 1.05, which means DEBE incurs less frequency leakage compared to CE.
cp ./script/exp1/constVar.h include/constVar.h
cp ./script/exp1/dataWriter.cc src/Server
cp ./script/exp1/restoreWriter.cc src/Client
cp ./script/exp1/storeEnclave.config.xml src/Enclave
bash ./recompile.sh
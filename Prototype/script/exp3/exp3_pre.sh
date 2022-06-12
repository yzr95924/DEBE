cp ./script/exp3/constVar.h include/constVar.h
cp ./script/exp3/dataWriter.cc src/Server
cp ./script/exp3/restoreWriter.cc src/Client
cp ./script/exp3/storeEnclave.config.xml src/Enclave
bash ./recompile.sh
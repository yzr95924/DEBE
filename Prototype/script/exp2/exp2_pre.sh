cp ./script/exp2/constVar.h include/constVar.h
cp ./script/exp2/dataWriter.cc src/Server
cp ./script/exp2/restoreWriter.cc src/Client
cp ./script/exp2/storeEnclave.config.xml src/Enclave
bash ./recompile.sh
cp ./script/exp6/constVar.h include/constVar.h
cp ./script/exp6/dataWriter.cc src/Server
cp ./script/exp6/restoreWriter.cc src/Client
cp ./script/exp6/storeEnclave.config.xml src/Enclave
bash ./recompile.sh
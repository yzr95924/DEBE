cp ./script/default/constVar.h include/constVar.h
cp ./script/default/dataWriter.cc src/Server
cp ./script/default/restoreWriter.cc src/Client
cp ./script/default/storeEnclave.config.xml src/Enclave
bash ./recompile.sh
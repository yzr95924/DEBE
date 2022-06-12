cp ./script/exp5/constVar.h include/constVar.h
cp ./script/exp5/dataWriter.cc src/Server
cp ./script/exp5/restoreWriter.cc src/Client
cp ./script/exp5/storeEnclave.config.xml src/Enclave
bash ./recompile.sh
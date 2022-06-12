cp ./script/exp4/constVar.h include/constVar.h
cp ./script/exp4/dataWriter.cc src/Server
cp ./script/exp4/restoreWriter.cc src/Client
cp ./script/exp4/storeEnclave.config.xml src/Enclave
bash ./recompile.sh
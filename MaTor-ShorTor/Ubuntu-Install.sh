sudo apt-get -y install libboost-all-dev
sudo apt-get -y install glpk-utils
sudo apt-get -y install cmake
sudo apt-get -y install libsqlite3-dev
sudo apt-get -y install golang
mkdir build
cd build
cmake ..
make
cd ../mator-db
export GOPATH="$0/mator-db/go"
bash installdb.sh
export PATH="${PATH}:$0/build/Release/lib"







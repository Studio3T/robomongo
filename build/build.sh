mkdir build
cd build
cmake ../../ -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX=/home/sasha -DBUILD_64X=0
make robomongo
make install

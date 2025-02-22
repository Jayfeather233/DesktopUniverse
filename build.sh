mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(( $(nproc) - 1 ))
cd ..
# ./build/DUS
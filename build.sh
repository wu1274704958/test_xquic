BuildType="Debug"
if [ $# -ge 1 ];then 
	BuildType=$1
fi
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=$BuildType -DCMAKE_TOOLCHAIN_FILE=/home/lighthouse/code/vcpkg/scripts/buildsystems/vcpkg.cmake -DSSL_PATH=/home/lighthouse/code/vcpkg/installed/x64-linux -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_MQAS_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON
cmake --build ./build

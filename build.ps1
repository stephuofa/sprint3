param(
    [switch]$clean,
    [switch]$release,
    [switch]$test
)

if($clean){
    rm -r build
    mkdir build
}

cd build

if($release){
    cmake -DCMAKE_BUILD_TYPE=Release ..
}else {
    cmake -DCMAKE_BUILD_TYPE=Debug ..
}

if($test){
    cmake -DMAKE_TESTS:BOOL=ON ..
}

cmake ..
cd ..
cmake --build build

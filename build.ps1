param(
    [switch]$clean
)

if($clean){
    rm -r build
    mkdir build
}

cd build
cmake ..
cd ..
cmake --build build

# run from top level (sprint3)


rm -rf build
mkdir build
cd build

for arg in "$@"; do
    if [ "$arg" = "-min" ]; then
        cmake -DMAKE_MIN:BOOL=ON ..
        break
    fi
done

cmake ..
make
cd ..
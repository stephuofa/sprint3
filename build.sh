# run from top level (sprint3)

for arg in "$@"; do
    if ["$arg" = "-clean" ]; then
        rm -rf build
        break
    fi
done

mkdir -p build
cd build

cmake -DCMAKE_BUILD_TYPE=Debug ..
for arg in "$@"; do
    if [ "$arg" = "-min" ]; then
        cmake -DMAKE_MIN:BOOL=ON ..
        continue
    fi
    if ["$arg" = "-test" ]; then
        cmake -DMAKE_TEST:BOOL=ON ..
        continue
    fi
    if ["$arg" = "-release" ]; then
        cmake -DCMAKE_BUILD_TYPE=Release ..
    fi
done

cmake ..
make
cd ..
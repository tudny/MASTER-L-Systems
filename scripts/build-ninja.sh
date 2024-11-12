BUILD_DIR=build-ninja

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -G "Ninja" ..
ninja

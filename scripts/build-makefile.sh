BUILD_DIR=build-makefile

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -G "Unix Makefiles" ..
make

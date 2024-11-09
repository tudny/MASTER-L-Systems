#!/bin/bash

set -e

GLSLANG_VALIDATOR_ZIP=glslang-main-linux-Release.zip
GLSLANG_VALIDATOR_URL=https://github.com/KhronosGroup/glslang/releases/download/main-tot/$GLSLANG_VALIDATOR_ZIP

cd "$(dirname "$0")/.."

if [ ! -f ./static/bin/glslangValidator ]; then
    echo "Downloading glslangValidator"
    curl -L $GLSLANG_VALIDATOR_URL -o $GLSLANG_VALIDATOR_ZIP
    unzip $GLSLANG_VALIDATOR_ZIP -d ./static
    rm $GLSLANG_VALIDATOR_ZIP
fi

function run_glslang_validator() {
    local file=$1
    echo "=============================="
    echo "| Checking $file"
    ./static/bin/glslangValidator $file
    echo "| Done"
    echo "=============================="
}

export -f run_glslang_validator

find . -type f \
  \( -name "*.vert" \
  -o -name "*.tesc" \
  -o -name "*.tese" \
  -o -name "*.geom" \
  -o -name "*.frag" \
  -o -name "*.comp" \) -exec bash -c 'run_glslang_validator "$0"' {} \;

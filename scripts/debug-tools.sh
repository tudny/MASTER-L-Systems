#!/bin/bash

set -e

RENDER_DOC_SOURCE_URL=https://renderdoc.org/stable/1.35/renderdoc_1.35.tar.gz
RENDER_DOC_BASE_DIR=debug-tools/renderdoc

cd "$(dirname "$0")/.."

if [ ! -d $RENDER_DOC_BASE_DIR ]; then
    echo "Downloading RenderDoc"
    mkdir -p $RENDER_DOC_BASE_DIR
    curl -L $RENDER_DOC_SOURCE_URL | tar -xz -C $RENDER_DOC_BASE_DIR --strip-components=1
fi

$RENDER_DOC_BASE_DIR/bin/renderdoccmd --version

#!/bin/bash

REPO_URL="https://github.com/ShiqiYu/libfacedetection.git"
TARGET_DIR="$(pwd)/libfacedetection"
BUILD_DIR="$TARGET_DIR/build"
INSTALL_DIR="$BUILD_DIR/install"

if [ ! -d "$TARGET_DIR" ]; then
    git clone "$REPO_URL" "$TARGET_DIR"
else
    echo "Repository already exists. Skipping clone."
fi

cd "$TARGET_DIR"
mkdir -p build
cd build

cmake .. -DCMAKE_INSTALL_PREFIX=install -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release -DDEMO=OFF
cmake --build . --config Release
cmake --build . --config Release --target install

cd ../..

ENV_FILE="$(pwd)/.env"
touch "$ENV_FILE"

if grep -q "^facedetection_DIR=" "$ENV_FILE"; then
    sed -i "s|^facedetection_DIR=.*|facedetection_DIR=$INSTALL_DIR|" "$ENV_FILE"
    echo "Updated existing facedetection_DIR in .env"
else
    echo "facedetection_DIR=$INSTALL_DIR" >> "$ENV_FILE"
    echo "Added facedetection_DIR to .env"
fi

export CMAKE_PREFIX_PATH="$BUILD_DIR:$CMAKE_PREFIX_PATH"

echo "Updated .env content:"
cat "$ENV_FILE"

echo "Build complete. facedetection_DIR set to $INSTALL_DIR in .env"

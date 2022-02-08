#!/bin/bash

set -x
set -e

APP_DIR=~
IMAGES_DIR=$APP_DIR/images/
IMAGES_S3_BUCKET=s3://libench/images
WWW_S3_BUCKET=s3://libench-website/
CODE_REPO=https://github.com/sandflow/libench.git
CODE_DIR=$APP_DIR/libench
BUILD_DIR=$CODE_DIR/build

# build libench

git clone --recurse-submodules $CODE_REPO $CODE_DIR

# initialize python

cd $CODE_DIR
pipenv install --system

# build libench

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
make -j 8

# load images

mkdir -p $IMAGES_DIR
cd $IMAGES_DIR
aws s3 sync $IMAGES_S3_BUCKET .

# run benchmark

cd $CODE_DIR
python3 src/main/python/make_page.py $IMAGES_DIR/manifest.json $BUILD_DIR/www

# upload website

aws s3 sync $BUILD_DIR/www $WWW_S3_BUCKET

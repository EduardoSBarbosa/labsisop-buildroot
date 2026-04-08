#!/bin/sh

echo ">>> POST BUILD EXECUTANDO"

cp $BASE_DIR/../custom-scripts/systeminfo.py \
   $TARGET_DIR/root/

cp $BASE_DIR/../custom-scripts/S50systeminfo \
   $TARGET_DIR/etc/init.d/

chmod +x $TARGET_DIR/etc/init.d/S50systeminfo
chmod +x $TARGET_DIR/root/systeminfo.py

#!/bin/sh

echo ">>> POST BUILD EXECUTANDO"

cp $BASE_DIR/../custom-scripts/systeminfo.py \
   $TARGET_DIR/root/

cp $BASE_DIR/../custom-scripts/disk_test \
   $TARGET_DIR/root/

cp $BASE_DIR/../modules/simple_driver/test_xtea_driver \
   $TARGET_DIR/root/

cp $BASE_DIR/../custom-scripts/disktest/disk_scheduler_test \
   $TARGET_DIR/root/

cp $BASE_DIR/../custom-scripts/disktest/data_extractor \
   $TARGET_DIR/root/

cp $BASE_DIR/../custom-scripts/disktest/data_creation.sh \
   $TARGET_DIR/root/

#cp $BASE_DIR/../custom-scripts/S50systeminfo \
#   $TARGET_DIR/etc/init.d/

#chmod +x $TARGET_DIR/etc/init.d/S50systeminfo
chmod +x $TARGET_DIR/root/disk_test
chmod +x $TARGET_DIR/root/systeminfo.py
chmod +x $TARGET_DIR/root/test_xtea_driver
chmod +x $TARGET_DIR/root/disk_scheduler_test
chmod +x $TARGET_DIR/root/data_extractor
chmod +x $TARGET_DIR/root/data_creation.sh
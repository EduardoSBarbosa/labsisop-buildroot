#!/bin/sh

#cp $BASE_DIR/../custom-scripts/S41network-config $BASE_DIR/target/etc/init.d
make -C $BASE_DIR/../modules/simple_driver/
i686-buildroot-linux-gnu-gcc $BASE_DIR/../custom-scripts/disktest/disktest.c -o $BASE_DIR/../custom-scripts/disktest/disk_test
i686-buildroot-linux-gnu-gcc $BASE_DIR/../custom-scripts/disktest/data_extractor.c -o $BASE_DIR/../custom-scripts/disktest/data_extractor
i686-buildroot-linux-gnu-gcc $BASE_DIR/../custom-scripts/disktest/disk_scheduler_test.c -o $BASE_DIR/../custom-scripts/disktest/disk_scheduler_test
#chmod +x $BASE_DIR/target/etc/init.d/S41network-config

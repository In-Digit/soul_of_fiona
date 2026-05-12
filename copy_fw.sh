#!/bin/bash
rm -rf /mnt/c/temp/fw
mkdir -p /mnt/c/temp/fw
cp build/bootloader/bootloader.bin /mnt/c/temp/fw/
cp build/partition_table/partition-table.bin /mnt/c/temp/fw/
cp build/lvgl_demo_v9.bin /mnt/c/temp/fw/
cp build/storage.bin /mnt/c/temp/fw/
ls -l /mnt/c/temp/fw/

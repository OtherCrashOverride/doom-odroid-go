#!/bin/bash
. ${IDF_PATH}/add_path.sh
FILESIZE=$(stat -c%s "$1")
#adjust filesize to a 4k multiple
SIZE=`echo "($FILESIZE + 4096)/4096*4096" | bc`;
esptool.py --chip esp32 --port "/dev/ttyUSB0" --before default_reset --after hard_reset erase_region 0x200000 $SIZE
esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud $((921600/2)) --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0x200000 "$1"

#!/bin/bash
. ${IDF_PATH}/add_path.sh
esptool.py --chip esp32 --port "/dev/ttyUSB0" --baud $((921600/2)) --before default_reset --after hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size detect 0x300000 doom1-cut.wad

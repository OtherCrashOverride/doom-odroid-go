#!/bin/bash
make
../odroid-go-firmware/tools/mkfw/mkfw "Doom SD" tile.raw 0 16 1048576 app build/esp32-doom.bin 66 6 4194304 wad doom1-cut.wad
mv firmware.fw Doom.fw

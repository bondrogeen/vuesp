#!/bin/bash

# $pio="~/.platformio/penv/bin/pio"
echo "Start build"
while getopts t:p:o: flag
do
    case "${flag}" in
        t) target=${OPTARG};;
        p) path=${OPTARG};;
        o) outdir=${OPTARG};;
    esac
done
echo "target: $target";
echo "path: $path";
echo "outdir: $outdir";

if [ "$1" = "build" ]
then
  echo 'build'
  ~/.platformio/penv/bin/pio run && 
  ~/.platformio/penv/bin/pio run --target buildfs &&
  cp ./build/esp8266/firmware.bin ./firmware/esp8266/ | cp ./build/esp8266/littlefs.bin ./firmware/esp8266/ &&
  cp ./build/esp32/firmware.bin ./firmware/esp32/ | cp ./build/esp32/littlefs.bin ./firmware/esp32/
fi

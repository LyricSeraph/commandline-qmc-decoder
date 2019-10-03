# QQMusic QMC Decoder (convert QMC File to MP3 or FLAC)

[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg?style=flat-square)](https://github.com/996icu/996.ICU/blob/master/LICENSE)
[![LICENSE](https://img.shields.io/badge/license-MIT-red.svg?style=flat-square)](https://github.com/Presburger/qmc-decoder/blob/master/LICENSE)


***SUPPORT QMC3/QMC0/QMCFLAC, Faster***

## Release

binary executable file is released [release](https://github.com/Presburger/qmc-decoder/releases)

## Build

* for linux

```shell
mkdir build
cd build
cmake ..
make
```

* for windows

```bat
mkdir build
cd build
cmake -G "NMake Makefiles" ..
nmake
```

## Convert
```
Usage: decoder [-o] [-r] (-M|-F) -f [<file> ...]
    -o: override exist output files.
    -r: recursively scan directories for matching files.
    -M: matching QMC3/QMC0 files
    -F: matching QMCFLAC files
    -f: directories/files to decode
```

* Todo

support auto fetch albums

support auto fix music meta data

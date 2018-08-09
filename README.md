# cart

[![license](https://img.shields.io/github/license/coord-e/cart.svg?style=flat-square)]()[![cpp](https://img.shields.io/badge/C%2B%2B-14-brightgreen.svg?style=flat-square)]()

Convert c/c++ code into compilable ascii art

# Getting started

## Docker

`docker run -v $(pwd):/data coorde/cart /data/code.cpp /data/img.png > out.cpp'

## Build from source

### Prerequirements
 - opencv
 - clang
 - [Taywee/args](https://github.com/Taywee/args)

### Clone, build, and install

```bash
git clone https://github.com/coord-e/cart
cd cart
mkdir build && cd $_
cmake ..
make
sudo make install
```

### Usage

```
cart [source] [image] {OPTIONS}

  cart: Convert a c/c++ code to ascii art

OPTIONS:

    -h, --help                        Print this help
    source                            Path to c source file
    image                             Path to image file
    -r[rows], --rows=[rows]           Number of rows (=5)
    -c[cols], --cols=[cols]           Number of cols (=5)
    --th=[threshold]                  threshold (=150)
    -d                                Use #define to shorten tokens
    --defth=[define threshold]        Minimum length of token (=5)
    -v, --verbose                     Print verbose output and show images in
                                      process
```

for example: `cart source.cpp image.png -r 2 -c 1 > out.cpp`

## Thanks to

Taywee/args - [https://github.com/Taywee/args](https://github.com/Taywee/args)

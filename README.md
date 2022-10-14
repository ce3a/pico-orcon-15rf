# PICO-ORCON-15RF

## Setting up the build environment

Get the  pico sdk.

```sh
git submodule update --init --recursive
```

Install the toolchain.

```sh
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

## Building the image

Build the image using CMake.

```sh
mkdir build
cd build
cmake ..
make install
```

If the build was successful, you will find the image in the `Release` directory.

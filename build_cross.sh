#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Define variables
TARGET="x86_64-elf"
PREFIX="$HOME/opt/cross"
PATH="$PREFIX/bin:$PATH"
BINUTILS_VERSION="2.44"
BINUTILS_VER_POSTFIX="tar.xz"
GCC_VERSION="14.2.0"
GCC_VER_POSTFIX="tar.gz"
CORES=$(nproc)  # Number of cores for parallel build

# Create necessary directories
mkdir -p $HOME/src $PREFIX

# Download and extract Binutils
cd $HOME/src
if [ ! -f binutils-$BINUTILS_VERSION.$BINUTILS_VER_POSTFIX ]; then
    echo "Downloading binutils-$BINUTILS_VERSION source..."
    curl -O https://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.$BINUTILS_VER_POSTFIX
fi
echo "Extracting binutils-$BINUTILS_VERSION.$BINUTILS_VER_POSTFIX..."
tar -xf binutils-$BINUTILS_VERSION.$BINUTILS_VER_POSTFIX


# Build and install Binutils
echo "Building binutils-$BINUTILS_VERSION..."
mkdir -p build-binutils
cd build-binutils
../binutils-$BINUTILS_VERSION/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror
make -j$CORES
make install

# Download and extract GCC
cd $HOME/src
if [ ! -f gcc-$GCC_VERSION.$GCC_VER_POSTFIX ]; then
    echo "Downloading gcc-$GC_VERSION source code..."
    curl -O https://ftp.gnu.org/gnu/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.$GCC_VER_POSTFIX
fi

echo "Extracting gcc-$GCC_VERSION.$GCC_VER_POSTFIX..."
tar -xf gcc-$GCC_VERSION.$GCC_VER_POSTFIX

# Download prerequisites for GCC
cd gcc-$GCC_VERSION
# ./contrib/download_prerequisites


# Build and install GCC
echo "Building gcc-$GCC_VERSION..."
mkdir -p $HOME/src/build-gcc
cd $HOME/src/build-gcc
../gcc-$GCC_VERSION/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make -j$CORES all-gcc
make -j$CORES all-target-libgcc
make install-gcc
make install-target-libgcc

echo "Cross-compiler for $TARGET installed to $PREFIX"

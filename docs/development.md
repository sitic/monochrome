# Contributing to Monochrome

Contributions are welcome! This document describes how to set up a development environment and how to contribute to the project.

## Installing from Source

Clone the repository and install the dependencies.

```bash
git clone https://github.com/sitic/monochrome.git
cd monochrome
```

### Dependencies

#### Linux

Install the following packages:
```bash
sudo apt install build-essential cmake mesa-utils xorg-dev
```

#### MacOS

Download [CMake](https://cmake.org/download/) and Xcode.

#### Windows

Download [CMake](https://cmake.org/) and a C++ compiler like Microsoft Visual C++.

### Compilation

#### Standalone Application

This project uses CMake to generate cross-platform build configurations.

```bash
# Create a build subfolder
mkdir build
cd build

# Generate a project for the default platform
cmake ..

# Then use your IDE or use CMake to compile
cmake --build .

# Linux only: installation
sudo cmake --build . --target install
```

#### Python Version

Alternatively, you can build the Python package which includes builing Monochrome with the following command:

```bash
python -m pip install .
```
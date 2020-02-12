# Quick Viewer for MultiRecorder and Binary Files

This is a simple viewer to play MultiRecorder and binary files. Drag & drop the file into the window or pass the paths as arguments.

## Build Requirements

### Linux 

```bash
sudo apt install build-essential cmake mesa-utils xorg-dev
```

### MacOS

Download [CMake](https://cmake.org/download/) and Xcode.

### Windows

Download [CMake](https://cmake.org/) and a C++ compiler like Microsoft Visual C++ or MinGW.

## Compilation

This project uses CMake to generate cross-platform build configurations. Either use cmake-gui or the terminal from the project folder (where CMakeList is located):

```bash
# Create a subfolder (generally called build)
mkdir build
cd build

# Generate a project for the default platform
cmake ..

# Alternatively display the available platforms and generate the project for the platform of your choice
cmake --help    # (optional)
cmake .. -G "Xcode"    # (example)
cmake .. -G "Unix Makefiles"    # (example)
cmake .. -G "Visual Studio 15 2017 Win64"   # (example)

# Then use your IDE or use CMake to compile
cmake --build .

# Linux only: installation
sudo cmake --build . --target install
```
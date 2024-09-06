# Tentacode
[Documentation](https://code.thehideoutgames.com/book/)

Public Games I've written in Tentacode:
- Highland Siege: https://syn9dev.itch.io/highland-siege

[Official Documentation](https://code.thehideoutgames.com/book/)

## Setup and Build Instructions

This guide provides the necessary steps to configure the tools required for building and running Tentacode.

### Prerequisites

Before proceeding, ensure that the following tools are installed:

- **CMake** (version 3.22 or higher)
- A **C++ compiler** with C++20 support
- **Git** (for fetching dependencies)

#### Windows Setup

For setting up your environment on Windows, follow the instructions for [w64devkit](https://github.com/raysan5/raylib/wiki/Working-on-Windows).

Additionally, download and install [CMake 3.22+](https://cmake.org/download/) for Windows.

#### Linux Setup

No additional setup instructions are specified for Linux. Ensure the required dependencies are installed using your package manager.

You can follow this [guide](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux) if you are having troubles. 

### Building and Running the Project

To build and run Tentacode, follow these steps:

```bash
cd tentacode
mkdir build
cd build
cmake ../
make -j
```

After the build completes, you can run the executable from the build/bin directory:

    ./bin/play

Alternatively, you can execute the binary from the project’s root directory: 

    ./play



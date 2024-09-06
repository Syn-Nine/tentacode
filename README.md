[Documentation](https://code.thehideoutgames.com/book/)


## Build and Run

To build and run the project using CMake, follow the steps below:

### Prerequisites

Ensure you have the following installed:

- **CMake** (version 3.16 or higher)
- **C++ compiler** that supports C++20
- **Git** (for fetching dependencies)


    cd tentacode
    mkdir build
    cd build
    cmake ../ -DUSE_RAYLIB=ON
    make -j4

Run the executable locating in the **build/bin**

    ./bin/tentacode_exe

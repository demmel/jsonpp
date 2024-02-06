## Dev Environment Setup

### Preequisites

- CMake
- Conan
- C++ Build Tools for your system

### Initializing the env

```
conan install . -of .\build\ --build=missing
cmake -B build -S . --preset conan-default
```

This will install the needed dependencies with Conan and setup the build with CMake.
If you make changes to conanfile.txt, you'll need to run these again.
If you make changes to CMakeLists.txt, you'll need to run cmake again.

## Building

```
cmake --build --preset conan-release .\build\
```

## Running Tests

```
ctest --preset conan-release
```

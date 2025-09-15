# mole

**WIP** - mole is a work in progress debugging utility.

mole enables custom tracking of memory allocations by hooking `malloc`/`free` upon loading the mole DLL. While the core hooking functionality works, there are limitations when using a DLL:

- Without a specialized injector, mole cannot capture the start and end of the target process, making it suitable primarily for applications like games.
- Long-standing allocations from the Windows runtime are filtered to avoid being misreported as memory leaks.

## Building mole

If you want to use mole, you will have to build it yourself, the esiest way is to run in the root directory:

```shell
./build.bat
```

For a release build, use:

```shell
./build.bat -release
```

This script sets up the MSVC environment and executes all necessary compilation commands.

### Manual Build with CMake
Alternatively, you can build Mole manually using CMake. No pre-installed dependencies are required beyond CMake and MSVC, as all dependencies are fetched by CMake.

### Visual Studio
CMake presets are provided for building Mole in Visual Studio. Simply open the project and use the provided presets to configure and build.
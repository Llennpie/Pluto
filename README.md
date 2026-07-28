# Plutoooooooooooooooooooooooooooooooooo

## Download

Prebuilt executables for Windows and Linux of Pluto can be downloaded from the [releases](https://github.com/Llennpie/Pluto/releases/latest) page

## Building

### Prerequisities

* [MSYS2](https://msys2.org) if you're on Windows
  * Pluto must be built in the **UCRT64** or **MINGW64** shell
  * The built-in updater will be disabled for MINGW64 builds
* `git` and `make`

### Compiling

* Clone the repository using git: `git clone https://github.com/Llennpie/Pluto`
  * Alternatively you can [download the source code](https://github.com/Llennpie/Pluto/archive/refs/heads/main.zip)
* Run `cd Pluto` to enter the Pluto source tree
* Run `make` (or `make -j$(nproc)` to speed up compilation at the cost of using more CPU power)
  * This also invokes your package manager to install necessary dependencies
* The built game will be located in the `build/us_pc` directory
  * On Windows, you can use the `explorer build/us_pc` to open File Explorer in that directory

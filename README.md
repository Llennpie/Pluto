# Plutoooooooooooooooooooooooooooooooooo

## Download

Pluto can be downloaded in the [releases section](https://github.com/Llennpie/Pluto/releases/latest).

For legal reasons, an unmodified, US SM64 ROM is needed to run Pluto.
This is to avoid including any copyrighted assets with the program.
You have to provide one yourself. **We cannot tell you how to get one.**

## Build Instructions

### Windows

* Download [MSYS2](https://msys2.org) from their official website
* After it's installed, go to the start menu and type in **MINGW64** and run it
  * Confirm that `MINGW64` is displayed in a magenta font
* Continue by following **Linux build instructions**.

### Linux

* Install dependencies
  * MINGW64/Windows: `pacman -S git make mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew mingw-w64-x86_64-openssl python3`
    * When pasting commands into the window, make sure to use the right click menu or **Shift+Insert** otherwise it won't work
    * When it prompts you, type in `y` and press Enter
  * Debian/Ubuntu/Mint: `sudo apt install git make gcc libsdl2-dev libglew-dev libssl-dev xclip`
  * Arch/CachyOS: `sudo pacman -S git make gcc sdl2-compat glew openssl xclip`
  * Gentoo: `emerge --ask dev-vcs/git dev-build/make media-libs/libsdl2 media-libs-glew dev-libs/openssl x11-misc/xclip`
* Download the source code: `git clone https://github.com/Llennpie/Pluto && cd Pluto`
* Run `make` to compile the program
  * You can significantly speed it up by using `make -j$(nproc)`, but it will use more of your CPU's processing power and make your computer slower while it compiles
* When it's done, return to the explorer window and go to the **build/us_pc** directory, and there's your Pluto build
* You can move the **dynos**, **mods**, **lang**, **bass.dll**, **bass_fx.dll**, **discord_game_sdk.dll** and **sm64coopdx.exe** into any folder you want and run it from there

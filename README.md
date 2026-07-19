# Plutoooooooooooooooooooooooooooooooooo

__**Windows:**__

* Download [MSYS2](<https://msys2.org>) from their official website
* After it's installed, go to the start menu and type in **MINGW64** and run it
  * Confirm that "MINGW64" is displayed in a magenta font
* Enter `pacman -S git make mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-glew mingw-w64-x86_64-cjson python3`
  * When pasting commands into the window, make sure to use the right click menu or **Shift+Insert** otherwise it won't work
  * When it prompts you, type in `y` and press Enter
* Download the source code: `git clone https://github.com/Llennpie/Pluto && cd Pluto`
* Enter `explorer .` (mind the period) to open File Explorer in the source code directory
* Move in an **umodified, US SM64 ROM** into the folder and rename it to **baserom.us.z64**
  * If you have file extensions disabled (which is the default on Windows), rename it to just **baserom.us**
* Return to the black window and enter `make` to compile the program
  * You can significantly speed it up by using `make -j$(nproc)`, but it will use more of your CPU's processing power and make your computer slower while it compiles
* When it's done, return to the explorer window and go to the **build/us_pc** directory, and there's your Pluto build
* You can move the **dynos**, **mods**, **lang**, **bass.dll**, **bass_fx.dll**, **discord_game_sdk.dll** and **sm64coopdx.exe** into any folder you want and run it from there

__**Linux:**__

* Open your terminal
* Install dependencies
  * Debian/Ubuntu/Mint: `sudo apt install git make gcc libsdl2-dev libglew-dev libcjson-dev xclip`
  * Arch/CachyOS: `sudo pacman -S git make gcc sdl2-compat glew cjson xclip`
  * Gentoo: `emerge --ask dev-vcs/git dev-build/make media-libs/libsdl2 media-libs-glew dev-libs/cJSON x11-misc/xclip`
* Run `git clone https://github.com/Llennpie/Pluto && cd Pluto`
* Open your file manager in the same folder
* Move in an **unmodified, US SM64 ROM** into the folder and rename it to **baserom.us.z64**
* Run `make` (or `make -j$(nproc)`)
* Once it completes, Pluto will be inside **build/us_pc**

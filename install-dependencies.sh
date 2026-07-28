#!/bin/sh

DEPENDENCIES_VERSION=1

if [ "$(cat .deps 2> /dev/null || echo 0)" -ge "$DEPENDENCIES_VERSION" ]; then exit 0; fi
exists() {
    command -v $1 > /dev/null 2>&1
}

find_sudo() {
    command -v sudo || command -v doas || (echo Cannot find sudo or doas; exit 1)
}

fail() {
    echo Unable to install required packages for building
    exit 1
}

echo Installing dependencies...

# detect platform

# MINGW64
if [ "$MSYSTEM" == "MINGW64" ]; then
    PREFIX=mingw-w64-x86_64
    pacman -S --noconfirm $PREFIX-gcc $PREFIX-SDL2 $PREFIX-glew $PREFIX-openssl $PREFIX-pkg-config python3 || fail
# UCRT64
elif [ "$MSYSTEM" == "UCRT64" ]; then
    PREFIX=mingw-w64-ucrt-x86_64
    pacman -S --noconfirm $PREFIX-gcc $PREFIX-SDL2 $PREFIX-glew $PREFIX-openssl $PREFIX-pkg-config python3 || fail
# Linux
elif [ -z "$MSYSTEM" ]; then
    SUDO=$(find_sudo)
    if exists apt; then # Ubuntu/Debian/Mint
        $SUDO apt install -y gcc libsdl2-dev libglew-dev libssl-dev pkg-config xclip python3 || fail
    elif exists pacman; then # Arch/CachyOS
        $SUDO pacman -S --noconfirm --needed gcc sdl2-compat glew openssl pkg-config xclip python3 || fail
    elif exists dnf; then # Fedora/RHEL
        $SUDO dnf install --assumeyes gcc sdl2-compat glew openssl pkg-config xclip python3 || fail
    elif exists emerge; then # Gentoo
        $SUDO emerge media-libs/libsdl2 media-libs-glew dev-libs/openssl dev-util/pkgconf x11-misc || fail
    elif exists zypper; then # OpenSUSE
        $SUDO zypper --non-interactive install gcc SDL2 glew openssl pkg-config xclip python3 || fail
    elif exists apk; then # Alpine
        $SUDO apk add gcc sdl2 glew openssl pkgconf xclip python3 || fail
    else
        echo Unable to find a package manager
        echo Please install the equivalent make, gcc, SDL2, glew, openssl, pkg-config, xclip and python packages
        echo $DEPENDENCIES_VERSION > .deps
        exit 1
    fi
else
    echo Please use either MINGW64 or UCRT64 environments
    exit 1
fi

echo $DEPENDENCIES_VERSION > .deps

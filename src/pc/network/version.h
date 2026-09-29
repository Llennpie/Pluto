#ifndef VERSION_H
#define VERSION_H

#define SM64COOPDX_VERSION "0.2"

#define VERSION_TEXT "beta"
#define VERSION_NUMBER 36
#define MINOR_VERSION_NUMBER 1
#define PATCH_VERSION_NUMBER 0

#if defined(VERSION_JP)
    #define VERSION_REGION "JP"
#elif defined(VERSION_EU)
    #define VERSION_REGION "EU"
#elif defined(VERSION_SH)
    #define VERSION_REGION "SH"
#else
    #define VERSION_REGION "US"
#endif

#define MAX_VERSION_LENGTH 48 //bump by 16 due to hashing, i don't think git uses a 40 bit hash, only a 7-12 so someone fact check this later
#define MAX_LOCAL_VERSION_LENGTH 52
const char* get_version(void);
const char* get_version_local(void);
const char* get_version_dx(void);
const char* get_version_pluto(void);
const char* get_game_name(void);

#endif

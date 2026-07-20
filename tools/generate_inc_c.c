#define PUSH_ASSET
#define SEPARATOR ,
#define ASSETS Asset assets[] =
#define STRUCT_ONLY

#include "src/saturn/asset_extractor/assets.c"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef _WIN32
#define mkdir(path) mkdir(path, 0744)
#endif

#define LENGTH(x) (sizeof(x) - 1)

char* parentdir(char* path) {
    for (int i = strlen(path) - 1; i >= 0; i-- ) {
        if (path[i] == '/') {
            path[i + 1] = 0;
            return path;
        }
    }
    return path;
}

void mkdir_if_not_exist(char* path) {
    struct stat st;
    if (stat(path, &st) == -1)
        mkdir(path);
}

void mkdirs(char* path) {
    int len = strlen(path);
    for (int i = 1; i < len; i++) {
        if (path[i] != '/') continue;

        path[i] = 0;
        mkdir_if_not_exist(path);
        path[i] = '/';
    }
}

void generate_file(const char* dst, const char* src) {
    if (access(dst, F_OK) == 0) return;
    
    int len = strlen(dst);
    char dirname[len + 1];
    strcpy(dirname, dst);
    mkdirs(parentdir(dirname));

    char c;
    FILE* f = fopen(dst, "w");
    while ((c = *src++))
        fprintf(f, "0x%02x,", c);
    fprintf(f, "0x00\n");
    fclose(f);
}

int main(int argc, char** argv) {
    if (argc == 1) {
        printf("Usage: %s base\n", argv[0]);
        return 1;
    }
    
    for (uint64_t i = 0; i < sizeof(assets) / sizeof(*assets); i++) {
        Asset* asset = &assets[i];

        if (
            strncmp(asset->name, "actors", 6) != 0 &&
            strncmp(asset->name, "levels", 6) != 0 &&
            strncmp(asset->name, "textures", 8) != 0
        ) continue;

        int dest_size = LENGTH("/.inc.c") + strlen(argv[1]) + strlen(asset->name) + 1;
        char filename[dest_size];
        sprintf(filename, "%s/%s.inc.c", argv[1], asset->name);
        generate_file(filename, asset->name);
    }

    return 0;
}

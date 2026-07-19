#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int extract_names(char* out_bank, char* out_sound, char* name) {
    if (sscanf(name, "sound/samples/%255[^/]/%255[^.].aiff", out_bank, out_sound) != 2) return 0;
    return 1;
}

void close_and_delete(FILE* f, const char* name) {
    fclose(f);
    remove(name);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s out.c out.h in...", argv[0]);
        return 1;
    }

    FILE* c = strcmp(argv[1], "-") == 0 ? stdout : fopen(argv[1], "w");
    if (!c) {
        perror("fopen out.c");
        return 1;
    }
    
    FILE* h = strcmp(argv[2], "-") == 0 ? stderr : fopen(argv[2], "w");
    if (!h) {
        perror("fopen out.h");
        close_and_delete(c, argv[1]);
    }

    fprintf(c, "#include \"custom_sounds.h\"\n");

    fprintf(h, "#include \"types.h\"\n");
    fprintf(h, "extern struct CustomSoundTable {\n");
    fprintf(h, "    const char* name;\n");
    fprintf(h, "    const u8* data;\n");
    fprintf(h, "    size_t size;\n");
    fprintf(h, "} gCustomSoundTable[%d];\n", argc - 3);

    for (int i = 3; i < argc; i++) {
        char bank_name[256], sound_name[256], path[256];
        if (!extract_names(bank_name, sound_name, argv[i])) {
            fprintf(stderr, "%s: invalid name\n", argv[i]);
            close_and_delete(c, argv[1]);
            close_and_delete(h, argv[2]);
        }
        sscanf(argv[i], "%255[^.].aiff", path);

        fprintf(c, "static const u8 sound_%s_%s[] = {\n", bank_name, sound_name);
        fprintf(c, "#include \"%s.aifc.inc.c\"\n", path);
        fprintf(c, "};\n");
    }

    fprintf(c, "struct CustomSoundTable gCustomSoundTable[%d] = {\n", argc - 3);
    for (int i = 3; i < argc; i++) {
        char bank_name[256], sound_name[256];
        if (!extract_names(bank_name, sound_name, argv[i])) {
            fprintf(stderr, "%s: invalid name\n", argv[i]);
            close_and_delete(c, argv[1]);
            close_and_delete(h, argv[2]);
        }

        fprintf(c, "    { \"%s/%s\", sound_%s_%s, sizeof(sound_%s_%s) },\n",
            bank_name, sound_name, bank_name, sound_name, bank_name, sound_name
        );
    }
    fprintf(c, "};\n");

    fclose(c);
    fclose(h);
    return 0;
}
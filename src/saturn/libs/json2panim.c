#include "json2panim.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include "cjson/cJSON.h"

int convert_count;

long find_length(char *i_file) {
  FILE *json_file_b = fopen(i_file, "rb");

  if (!json_file_b) {
    perror("File failed to open");
    return -1;
  }

  fseek(json_file_b, 0, SEEK_END);
  long len = ftell(json_file_b);

  fclose(json_file_b);

  return len;
}

void convert_mcomp_to_panim(char *i_file, char *o_file) {
  FILE *json_file = fopen(i_file, "r");

  if (!json_file) {
    perror("File failed to open");
    return;
  }

  char *buf = malloc(find_length(i_file));
  size_t read_file = fread(buf, sizeof(buf[0]), find_length(i_file), json_file);
  fclose(json_file);

  cJSON *json = cJSON_Parse(buf);
  if (json == NULL) {
    const char *error_p = cJSON_GetErrorPtr();
    if (error_p != NULL) {
      printf("Error: %s\n", error_p);
    }

    cJSON_Delete(json);
    return;
  }

  cJSON *name = cJSON_GetObjectItemCaseSensitive(json, "name");
  cJSON *author = cJSON_GetObjectItemCaseSensitive(json, "author");
  cJSON *looping = cJSON_GetObjectItemCaseSensitive(json, "looping");
  cJSON *length = cJSON_GetObjectItem(json, "length");
  cJSON *nodes = cJSON_GetObjectItem(json, "nodes");
  cJSON *values = cJSON_GetObjectItem(json, "values");
  uint8_t bval[cJSON_GetArraySize(values)];
  if (cJSON_IsArray(values)) {
    const cJSON *value = NULL;
    int i = 0;
    cJSON_ArrayForEach(value, values) {
      bval[i] = strtol(value->valuestring, NULL, 16);
      i++;
    }
  }
  cJSON *indices = cJSON_GetObjectItem(json, "indices");
  uint8_t bind[cJSON_GetArraySize(indices)];
  if (cJSON_IsArray(indices)) {
    const cJSON *value = NULL;
    int i = 0;
    cJSON_ArrayForEach(value, indices) {
      bind[i] = strtol(value->valuestring, NULL, 16);
      i++;
    }
  }
  
  FILE *panim_file = fopen(o_file, "wb");

  fwrite(name->valuestring, sizeof(char), strlen(name->valuestring), panim_file);
  for (int i = 0; i < 64 - strlen(name->valuestring); i++) {
    fputc(0, panim_file);
  }

  fwrite(author->valuestring, sizeof(char), strlen(author->valuestring), panim_file);
  for (int i = 0; i < 32 - strlen(author->valuestring); i++) {
    fputc(0, panim_file);
  }

  if (!strcmp(looping->valuestring, "true")) {
    fputc(166, panim_file);
  } else {
    fputc(0, panim_file);
  }

  fputc(((uint16_t)cJSON_GetNumberValue(length) >> 8) & 0xFF, panim_file);
  fputc((uint16_t)cJSON_GetNumberValue(length) & 0xFF, panim_file);

  uint8_t bnodes = (uint8_t)cJSON_GetNumberValue(nodes) > 255 ? 255 : (uint8_t)cJSON_GetNumberValue(nodes);
  fputc(bnodes, panim_file);

  fwrite("values", sizeof(char), 6, panim_file);
  fwrite(bval, sizeof(uint8_t), sizeof(bval)/sizeof(bval[0]), panim_file);

  fwrite("indices", sizeof(char), 7, panim_file);
  fwrite(bind, sizeof(uint8_t), sizeof(bind)/sizeof(bind[0]), panim_file);

  cJSON_Delete(json);
  fclose(panim_file);

  convert_count++;
}
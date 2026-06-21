#include "test.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char **test_1_svc(char **input, struct svc_req *req) {
    static char *result;
    char *str = *input;

    result = malloc(strlen(str) + 1);
    if (!result) return NULL;

    for (int i = 0; str[i]; i++)
        result[i] = toupper((unsigned char)str[i]);
    result[strlen(str)] = '\0';

    return &result;
}

#include "test.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    CLIENT *clnt;
    char *server = "localhost";
    char *input = "hello rpc";
    char **result;

    clnt = clnt_create(server, TEST_PROG, TEST_VERS, "tcp");
    if (!clnt) {
        clnt_pcreateerror(server);
        return 1;
    }

    result = test_1(&input, clnt);
    if (!result) {
        clnt_perror(clnt, "call failed");
        return 1;
    }

    printf("Original: %s\n", input);
    printf("Result  : %s\n", *result);

    clnt_freeres(clnt, (xdrproc_t)xdr_wrapstring, (char*)result);
    clnt_destroy(clnt);
    return 0;
}

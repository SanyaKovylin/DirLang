#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>

#include "utils.h"

size_t Read (const char *src, char **Buffer) {

    assert (src    != NULL);
    assert (Buffer != NULL);

    int fo = open (src, O_RDONLY | O_BINARY);

    struct stat st = {};

    stat (src, &st);
    _off_t readlen = st.st_size;

    *Buffer = (char*) calloc (readlen + 1, sizeof (char));

    size_t lenbuf = read (fo, *Buffer, readlen);
    return lenbuf;
}

void *Resize(void *src, size_t size, size_t lase_n, bool upscale){

    assert(src != NULL);

    src = realloc (src, (int) lase_n*size*ResizeScale);
    assert(src != NULL);

    if (upscale)
        memset((char*) src + lase_n*size, '\0', (size_t) lase_n*size*(ResizeScale - 1));

    return src;
}

const double eps = 10e-6;
bool IsZero(double val){
    return abs(val) < eps;
}

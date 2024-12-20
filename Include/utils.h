#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

struct Buffer{
    char *buffer;
    size_t readptr;
    size_t len;
};

size_t Read (const char *src, char **Buffer);

void *Resize(void *src, size_t size, size_t last_n, bool upscale);
const int ResizeScale = 2;

bool IsZero(double val);

#endif //UTILS_H_INCLUDED

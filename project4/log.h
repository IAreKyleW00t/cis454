#pragma once
#include <stdbool.h>

bool lopen(char *);
bool lclose(void);
ssize_t lwrite(const void *, size_t);
#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct selector_s selector_t;

selector_t *selector_new(uint64_t id, const char *name, size_t name_len);

const char *selector_get_name(const selector_t *selector);

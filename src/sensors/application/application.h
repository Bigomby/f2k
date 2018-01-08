#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct application_s application_t;

application_t *application_new(uint64_t id, const char *name, size_t name_len);

const char *application_get_name(const application_t *application);

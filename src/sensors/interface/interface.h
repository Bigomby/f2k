#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct interface_s interface_t;

interface_t *interface_new(uint64_t id, const char *name, size_t name_len,
                           const char *description, size_t description_len);

const char *interface_get_name(const interface_t *interface);

const char *interface_get_description(const interface_t *interface);

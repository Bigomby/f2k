#pragma once

#include <stdint.h>

typedef struct network_s network_t;

network_t *network_new(uint8_t network[16], uint8_t netmask[16],
                       const char *name);

const char *network_get_name(const network_t *network);

const char *network_get_ip_str(const network_t *network);

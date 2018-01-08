#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct sensor_s sensor_t;
typedef struct observation_id_s observation_id_t;

const char *sensor_get_network_string(const sensor_t *sensor);

uint32_t *sensor_get_observation_id_list(const sensor_t *sensor,
                                         size_t *list_length);

observation_id_t *sensor_get_observation_id(sensor_t *sensor, uint32_t id);

observation_id_t *sensor_get_default_observation_id(sensor_t *sensor);

void *sensor_get_worker(const sensor_t *sensor);

void sensor_set_worker(sensor_t *sensor, void *worker);

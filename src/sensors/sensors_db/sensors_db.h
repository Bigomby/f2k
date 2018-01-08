#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct sensor_s sensor_t;
typedef struct sensors_db_s sensors_db_t;
typedef struct observation_id_s observation_id_t;

sensors_db_t *sensors_db_new();

sensor_t *sensors_db_get(sensors_db_t *db, const uint8_t network[16]);

void sensors_db_add(sensors_db_t *db, sensor_t *sensor);

sensor_t **sensors_db_list(const sensors_db_t *db, size_t *list_length);

void sensors_db_destroy(sensors_db_t *db);

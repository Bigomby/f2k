#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct file_loader_s file_loader_t;
typedef struct sensor_s sensor_t;
typedef void (*new_sensor_event_t)(sensor_t *, void *);

file_loader_t *db_loader_new_file_loader();

void db_loader_set_new_sensor_event(file_loader_t *file_loader,
                                    new_sensor_event_t event_cb, void *ctx);

void db_loader_load_file(file_loader_t *file_loader, const char *filename);

void db_loader_file_loader_destroy(file_loader_t *file_loader);

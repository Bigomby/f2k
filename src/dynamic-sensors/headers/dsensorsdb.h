#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct sensor_s sensor_t;
typedef struct sensors_db_s sensors_db_t;
typedef struct observation_id_s observation_id_t;
typedef struct network_s network_t;
typedef struct application_s application_t;
typedef struct interface_s interface_t;
typedef struct selector_s selector_t;

////////////////////////////////////////////////////////////////////////////////
// Sensors DB
////////////////////////////////////////////////////////////////////////////////

sensors_db_t *sensors_db_new();

void sensors_db_destroy(sensors_db_t *db);

sensor_t **sensors_db_list(const sensors_db_t *db, size_t *list_length);

sensor_t *sensors_db_get(const sensors_db_t *db, const uint8_t network[16]);

void sensors_db_add(sensors_db_t *db, sensor_t *sensor);

////////////////////////////////////////////////////////////////////////////////
// Sensor
////////////////////////////////////////////////////////////////////////////////

sensor_t *sensor_new(const uint8_t network[16], const uint8_t netmask[16]);

const char *sensor_get_network_string(const sensor_t *sensor);

uint32_t *sensor_get_observation_id_list(const sensor_t *sensor,
                                         size_t *list_length);

observation_id_t *sensor_get_observation_id(sensor_t *sensor, uint32_t id);

observation_id_t *sensor_get_default_observation_id(sensor_t *sensor);

void *sensor_get_worker(const sensor_t *sensor);

void sensor_set_worker(sensor_t *sensor, void *worker);

void sensor_add_observation_id(sensor_t *sensor,
                               observation_id_t *observation_id);

void sensor_add_default_observation_id(sensor_t *sensor,
                                       observation_id_t *observation_id);

////////////////////////////////////////////////////////////////////////////////
// Observation ID
////////////////////////////////////////////////////////////////////////////////

observation_id_t *observation_id_new(uint32_t id);

uint32_t observation_id_get_id(const observation_id_t *observation_id);

const network_t *
observation_id_get_network(const observation_id_t *observation_id,
                           const uint8_t ip[16]);

const selector_t *
observation_id_get_selector(const observation_id_t *observation_id,
                            uint64_t selector_id);

const application_t *
observation_id_get_application(const observation_id_t *observation_id,
                               uint64_t application_id);

const interface_t *
observation_id_get_interface(const observation_id_t *observation_id,
                             uint64_t interface_id);

int64_t observation_id_get_fallback_first_switch(
    const observation_id_t *observation_id);

uint16_t *observation_id_list_templates(const observation_id_t *observation_id,
                                        size_t *list_length);

void *observation_id_get_template(const observation_id_t *observation_id,
                                  uint16_t template_id);

const char *
observation_id_get_enrichment(const observation_id_t *observation_id);

bool observation_id_want_client_dns(const observation_id_t *observation_id);

bool observation_id_want_target_dns(const observation_id_t *observation_id);

bool observation_id_is_exporter_in_wan_side(
    const observation_id_t *observation_id);

bool observation_id_is_span_port(const observation_id_t *observation_id);

void observation_id_add_template(observation_id_t *observation_id, uint16_t id,
                                 void *tmpl);

void observation_id_add_application(observation_id_t *observation_id,
                                    const application_t *application);

void observation_id_add_selector(observation_id_t *observation_id,
                                 const selector_t *selector);

void observation_id_add_interface(observation_id_t *observation_id,
                                  const interface_t *interface);

void observation_id_add_network(observation_id_t *observation_id,
                                const network_t *network);

void observation_id_set_enrichment(const observation_id_t *observation_id,
                                   const char *enrichment);

void observation_id_set_fallback_first_switch(observation_id_t *observation_id,
                                              int64_t fallback_first_switch);

void observation_id_set_exporter_in_wan_side(observation_id_t *observation_id);

void observation_id_set_span_mode(observation_id_t *observation_id);

void observation_id_enable_ptr_dns_client(observation_id_t *observation_id);

void observation_id_enable_ptr_dns_target(observation_id_t *observation_id);

////////////////////////////////////////////////////////////////////////////////
// Network
////////////////////////////////////////////////////////////////////////////////

network_t *network_new(uint8_t network[16], uint8_t netmask[16],
                       const char *name);

const char *network_get_name(const network_t *network);

const char *network_get_ip_str(const network_t *network);

////////////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////////////

interface_t *interface_new(uint64_t id, const char *name, size_t name_len,
                           const char *description, size_t description_len);

const char *interface_get_name(const interface_t *interface);

const char *interface_get_description(const interface_t *interface);

////////////////////////////////////////////////////////////////////////////////
// Application
////////////////////////////////////////////////////////////////////////////////

application_t *application_new(uint64_t id, const char *name, size_t name_len);

const char *application_get_name(const application_t *application);

////////////////////////////////////////////////////////////////////////////////
// Selector
////////////////////////////////////////////////////////////////////////////////

selector_t *selector_new(uint64_t id, const char *name, size_t name_len);

const char *selector_get_name(const selector_t *selector);

////////////////////////////////////////////////////////////////////////////////
// Util
////////////////////////////////////////////////////////////////////////////////

void dsensors_free(void *ptr);

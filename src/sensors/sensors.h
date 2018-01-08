#pragma once

#include "../f2k.h"
#include "sensors_db/sensors_db.h"

typedef struct sensors_db_s sensors_db_t;
typedef struct sensor_s sensor_t;
typedef struct observation_id_s observation_id_t;
typedef struct worker_s worker_t;

////////////
// Events //
////////////

void add_new_sensor_event(sensor_t *sensor, void *ctx);

//////////////
// Database //
//////////////

void delete_rb_sensors_db(sensors_db_t *database);

////////////
// Sensor //
////////////

sensor_t *get_sensor(sensors_db_t *database, uint64_t ip);

const char *sensor_ip_string(const sensor_t *sensor);

observation_id_t *get_sensor_observation_id(sensor_t *sensor, uint32_t obs_id);

worker_t *sensor_worker(const sensor_t *sensor);

void set_workers(sensors_db_t *db, worker_t **worker_list,
                 size_t worker_list_size);

int addBadSensor(sensors_db_t *database, uint64_t sensor_ip);

////////////////////
// Observation ID //
////////////////////

uint32_t observation_id_num(const observation_id_t *observation_id);

int64_t
observation_id_fallback_first_switch(const observation_id_t *observation_id);

bool is_exporter_in_wan_side(const observation_id_t *observation_id);

bool is_span_observation_id(const observation_id_t *observation_id);

const struct flowSetV9Ipfix *
find_observation_id_template(const observation_id_t *observation_id,
                             const uint16_t template_id);

const char *observation_id_enrichment(const observation_id_t *obs_id);

const char *observation_id_application_name(observation_id_t *observation_id,
                                            uint64_t application_id);

const char *observation_id_selector_name(observation_id_t *observation_id,
                                         uint64_t selector_id);

const char *observation_id_interface_name(observation_id_t *observation_id,
                                          uint64_t interface_id);

const char *network_name(observation_id_t *obs_id, const uint8_t ip[16]);

const char *network_ip(observation_id_t *obs_id, const uint8_t ip[16]);

const char *
observation_id_interface_description(observation_id_t *observation_id,
                                     uint64_t interface_id);

void observation_id_add_application_id(observation_id_t *observation_id,
                                       uint64_t application_id,
                                       const char *application_name,
                                       size_t application_name_len);

void observation_id_add_selector_id(observation_id_t *observation_id,
                                    uint64_t selector_id,
                                    const char *selector_name,
                                    size_t selector_name_len);

void observation_id_add_new_interface(observation_id_t *observation_id,
                                      uint64_t interface_id,
                                      const char *interface_name,
                                      size_t interface_name_len,
                                      const char *interface_description,
                                      size_t interface_description_len);

/**
 * Store a template inside an observation ID. Calling this function tranfers
 * the ownership to the observation_id.
 *
 * @param observation_id Observation ID to store the template.
 * @param tmpl           Template to store.
 */
void save_template(observation_id_t *observation_id,
                   const struct flowSetV9Ipfix *tmpl);

void save_template_async(sensor_t *sensor, struct flowSetV9Ipfix *tmpl);

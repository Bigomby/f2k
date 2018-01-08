#include "application/application.h"
#include "interface/interface.h"
#include "network/network.h"
#include "observation_id/observation_id.h"
#include "selector/selector.h"
#include "sensor/sensor.h"
#include "util.h"
#include "../db_loader/file_loader/file_loader.h"
#include "sensors.h"

#include "../util.h"

#include <jansson.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

////////////
// Events //
////////////

void add_new_sensor_event(sensor_t *sensor, void *ctx) {
  assert(sensor);
  assert(ctx);

  sensors_db_add((sensors_db_t *)ctx, sensor);
}

//////////////
// Database //
//////////////

inline void delete_rb_sensors_db(sensors_db_t *database) {
  sensors_db_destroy(database);
}

////////////
// Sensor //
////////////

inline sensor_t *get_sensor(sensors_db_t *database, uint64_t ip) {
  uint8_t ip_address[16] = {0};
  ip_address[10] = 0xff;
  ip_address[11] = 0xff;
  ip_address[12] = ip >> 24;
  ip_address[13] = ip >> 16;
  ip_address[14] = ip >> 8;
  ip_address[15] = ip;

  return sensors_db_get(database, ip_address);
}

inline const char *sensor_ip_string(const sensor_t *sensor) {
  return sensor_get_network_string(sensor);
}

inline observation_id_t *get_sensor_observation_id(sensor_t *sensor,
                                                   uint32_t obs_id) {
  return sensor_get_observation_id(sensor, obs_id);
}

inline worker_t *sensor_worker(const sensor_t *sensor) {
  return sensor_get_worker(sensor);
}

inline void set_workers(sensors_db_t *db, worker_t **worker_list,
                        size_t worker_list_size) {
  assert(db);
  assert(worker_list);

  size_t list_length = 0;
  sensor_t **sensors = sensors_db_list(db, &list_length);

  for (size_t i = 0; i < list_length; i++) {
    sensor_set_worker(sensors[i], worker_list[i % worker_list_size]);
  }

  dsensors_free(sensors);
}

inline int addBadSensor(sensors_db_t *database, uint64_t sensor_ip) {
  return 0;
}

////////////////////
// Observation ID //
////////////////////

inline uint32_t observation_id_num(const observation_id_t *observation_id) {
  return observation_id_get_id(observation_id);
}

inline int64_t
observation_id_fallback_first_switch(const observation_id_t *observation_id) {
  return observation_id_get_fallback_first_switch(observation_id);
}

inline bool is_exporter_in_wan_side(const observation_id_t *observation_id) {
  return observation_id_is_exporter_in_wan_side(observation_id);
}

inline bool is_span_observation_id(const observation_id_t *observation_id) {
  return observation_id_is_span_port(observation_id);
}

// TODO
inline const struct flowSetV9Ipfix *
find_observation_id_template(const observation_id_t *observation_id,
                             const uint16_t template_id) {
  return observation_id_get_template(observation_id, template_id);
}

inline const char *observation_id_enrichment(const observation_id_t *obs_id) {
  return observation_id_get_enrichment(obs_id);
}

inline const char *
observation_id_application_name(observation_id_t *observation_id,
                                uint64_t application_id) {
  const application_t *application =
      observation_id_get_application(observation_id, application_id);
  if (!application) {
    return NULL;
  }

  return application_get_name(application);
}

inline const char *
observation_id_selector_name(observation_id_t *observation_id,
                             uint64_t selector_id) {
  const selector_t *selector =
      observation_id_get_selector(observation_id, selector_id);
  if (!selector) {
    return NULL;
  }

  return selector_get_name(selector);
}

inline const char *
observation_id_interface_name(observation_id_t *observation_id,
                              uint64_t interface_id) {
  const interface_t *interface =
      observation_id_get_interface(observation_id, interface_id);
  if (!interface) {
    return NULL;
  }

  return interface_get_name(interface);
}

const char *network_name(observation_id_t *obs_id, const uint8_t ip[16]) {
  const network_t *network = observation_id_get_network(obs_id, ip);
  if (!network) {
    return NULL;
  }

  return network_get_name(network);
}

const char *network_ip(observation_id_t *obs_id, const uint8_t ip[16]) {
  const network_t *network = observation_id_get_network(obs_id, ip);
  if (!network) {
    return NULL;
  }

  return network_get_ip_str(network);
}

inline const char *
observation_id_interface_description(observation_id_t *observation_id,
                                     uint64_t interface_id) {
  const interface_t *interface =
      observation_id_get_interface(observation_id, interface_id);
  if (!interface) {
    return NULL;
  }

  return interface_get_description(interface);
}

inline void observation_id_add_application_id(observation_id_t *observation_id,
                                              uint64_t application_id,
                                              const char *application_name,
                                              size_t application_name_len) {
  application_t *application =
      application_new(application_id, application_name, application_name_len);
  if (!application) {
    return;
  }

  observation_id_add_application(observation_id, application);
}

inline void observation_id_add_selector_id(observation_id_t *observation_id,
                                           uint64_t selector_id,
                                           const char *selector_name,
                                           size_t selector_name_len) {
  selector_t *selector =
      selector_new(selector_id, selector_name, selector_name_len);
  if (!selector) {
    return;
  }

  observation_id_add_selector(observation_id, selector);
}

inline void observation_id_add_new_interface(observation_id_t *observation_id,
                                             uint64_t interface_id,
                                             const char *interface_name,
                                             size_t interface_name_len,
                                             const char *interface_description,
                                             size_t interface_description_len) {

  interface_t *interface =
      interface_new(interface_id, interface_name, interface_name_len,
                    interface_description, interface_description_len);
  if (!interface) {
    return;
  }

  observation_id_add_interface(observation_id, interface);
}

inline void save_template(observation_id_t *observation_id,
                          const struct flowSetV9Ipfix *template) {
  assert(observation_id);
  assert(template);

  const V9IpfixSimpleTemplate *templateInfo = &template->templateInfo;

  struct flowSetV9Ipfix *new_template = NULL;
  rd_calloc_struct(&new_template, sizeof(struct flowSetV9Ipfix),
                   template->templateInfo.fieldCount *
                       sizeof(template->fields[0]),
                   template->fields, &new_template->fields, RD_MEM_END_TOKEN);

  if (!new_template) {
    traceEvent(TRACE_ERROR, "Not enough memory");
    return;
  }

  new_template->templateInfo.templateId = template->templateInfo.templateId;
  new_template->templateInfo.fieldCount = template->templateInfo.fieldCount;
  new_template->templateInfo.is_option_template =
      template->templateInfo.is_option_template;
  new_template->templateInfo.netflow_device_ip =
      template->templateInfo.netflow_device_ip;
  new_template->templateInfo.observation_domain_id =
      template->templateInfo.observation_domain_id;

  if (!template->templateInfo.is_option_template) {
    for (int fieldId = 0; fieldId < new_template->templateInfo.fieldCount;
         ++fieldId) {
      const uint16_t entity_id = new_template->fields[fieldId].fieldId;
      new_template->fields[fieldId].v9_template = find_template(entity_id);
    }
  }

  observation_id_add_template(observation_id, templateInfo->templateId,
                              new_template);
}

void save_template_async(sensor_t *sensor, struct flowSetV9Ipfix *tmpl) {
  // TODO
}

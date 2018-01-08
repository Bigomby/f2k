#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct observation_id_s observation_id_t;
typedef struct network_s network_t;
typedef struct application_s application_t;
typedef struct interface_s interface_t;
typedef struct selector_s selector_t;

uint32_t observation_id_get_id(const observation_id_t *observation_id);

void *observation_id_get_template(const observation_id_t *observation_id,
                                  uint16_t template_id);

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

const uint16_t *
observation_id_list_templates(const observation_id_t *observation_id,
                              size_t *list_length);

const char *
observation_id_get_enrichment(const observation_id_t *observation_id);

bool observation_id_want_client_dns(const observation_id_t *observation_id);

bool observation_id_want_target_dns(const observation_id_t *observation_id);

bool observation_id_is_exporter_in_wan_side(
    const observation_id_t *observation_id);

bool observation_id_is_span_port(const observation_id_t *observation_id);

void observation_id_add_application(observation_id_t *observation_id,
                                    const application_t *application);

void observation_id_add_selector(observation_id_t *observation_id,
                                 const selector_t *selector);

void observation_id_add_interface(observation_id_t *observation_id,
                                  const interface_t *interface);

void observation_id_add_network(observation_id_t *observation_id,
                                const network_t *network);

void observation_id_set_enrichment(observation_id_t *observation_id,
                                   const char *enrichment);

void observation_id_add_template(observation_id_t *observation_id, uint16_t id,
                                 void *template_ptr);

void observation_id_set_fallback_first_switch(observation_id_t *observation_id,
                                              int64_t fallback_first_switch);

void observation_id_set_exporter_in_wan_side(observation_id_t *observation_id);

void observation_id_set_span_mode(observation_id_t *observation_id);

void observation_id_enable_ptr_dns_client(observation_id_t *observation_id);

void observation_id_enable_ptr_dns_target(observation_id_t *observation_id);

/*
  Copyright (C) 2016 Eneo Tecnologia S.L.
  Author: Eugenio Perez <eupm90@gmail.com>
  Based on Luca Deri nprobe 6.22 collector

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "export.h"
#include "util.h"
#include "rb_mac.h"
#include "rb_sensor.h"

#ifdef HAVE_UDNS
#include "rb_dns_cache.h"
#endif

#include <librd/rdfile.h>

#define NETFLOW_DIRECTION_INGRESS 0
#define NETFLOW_DIRECTION_EGRESS  1

#define assert_multi(...) do {size_t assert_i; \
  for(assert_i=0; \
      assert_i<sizeof((const void *[]){__VA_ARGS__})/sizeof(const void *);\
      ++assert_i) {assert(((const void *[]){__VA_ARGS__})[assert_i]);}}while(0)

/// Get rid of unused parameters
static void unused_params0(const void *p,...) {(void)p;}
#define unused_params(p...) unused_params0(&p)

typedef enum{
  DIRECTION_UNSET,
  DIRECTION_INGRESS,
  DIRECTION_EGRESS,
  DIRECTION_INTERNAL,
} direction_t;

struct flowCache *new_flowCache(){
  return calloc(1,sizeof(struct flowCache));
}

void associateSensor(struct flowCache *flowCache, struct sensor *sensor){
  assert_multi(flowCache, sensor);

  flowCache->sensor = sensor;
}

void free_flowCache(struct flowCache *cache){
  free(cache);
}

static int ip_direction(int known_src,int known_dst) {
  if(!known_src && known_dst) {
    return DIRECTION_EGRESS;
  } else if(known_src && !known_dst) {
    return DIRECTION_INGRESS;
  } else if(known_src && known_dst) {
    return DIRECTION_INTERNAL;
  }
  // No "external"
  return DIRECTION_UNSET;
}

static int mac_direction(int known_src,int known_dst) {
  if(known_src && !known_dst) {
    return DIRECTION_EGRESS;
  } else if(!known_src && known_dst) {
    return DIRECTION_INGRESS;
  }

  // else no idea
  return DIRECTION_UNSET;
}

/*
  Try to guess direction based on source and destination address
  return: 1 if guessed/already setted. 0 if couldn't set
*/
int guessDirection(struct flowCache *cache){
  assert(cache);
  static const char zeros[sizeof(cache->address.src)] = {0};
  // Don't trust in flow direction unless there is no choice
  // if(cache->macs.direction != DIRECTION_UNSET){
  //  return 1; /* no need to guess */
  //}

  if(NULL == cache) {
    traceEvent(TRACE_ERROR,"Invalid address");
    return 0;
  }

  if(sensor_has_mac_db(cache->sensor)) {
    const uint64_t src_mac_as_num = net2number(cache->macs.src_mac, 6);
    const uint64_t dst_mac_as_num = is_span_sensor(cache->sensor) ?
      net2number(cache->macs.dst_mac, 6) :
      net2number(cache->macs.post_dst_mac, 6);

    const int src_mac_is_router = sensor_has_router_mac(cache->sensor,src_mac_as_num);
    const int dst_mac_is_router = sensor_has_router_mac(cache->sensor,dst_mac_as_num);

    if(0!=src_mac_as_num && valid_mac(src_mac_as_num)
       && 0!=dst_mac_as_num && valid_mac(dst_mac_as_num)) {

      const int mac_guessed_direction = mac_direction(src_mac_is_router,dst_mac_is_router);
      if(mac_guessed_direction != DIRECTION_UNSET) {
        cache->macs.direction = mac_guessed_direction;
        return 1;
      }
    }
  }

  if (0 == memcmp(cache->address.src, zeros, sizeof(cache->address.src)) ||
      0 == memcmp(cache->address.dst, zeros, sizeof(cache->address.dst))) {
    /* Can't guess direction */
    return -1;
  }

  const int src_ip_in_home_net = NULL!=network_ip(cache->sensor,cache->address.src);
  const int dst_ip_in_home_net = NULL!=network_ip(cache->sensor,cache->address.dst);

  const int ip_guessed_direction = ip_direction(src_ip_in_home_net,dst_ip_in_home_net);
  if(ip_guessed_direction != DIRECTION_UNSET) {
    cache->macs.direction = ip_guessed_direction;
    return 1;
  }

  return 1;
}

#if 0
/* Just for templating */
/*
 * Function: C++ version 0.4 char* style "itoa", Written by Lukás Chmela. (Modified)
 *
 * Purpose: Fast itoa conversion. snprintf is slow.
 *
 * Arguments:   value => Number.
 *             result => Where to save result
 *               base => Number base.
 *
 * Return: result
 * TODO: Return writed buffer lenght.
 *
 */
static char* _itoa(uint64_t value, char* result, int base, size_t bufsize) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char *ptr = result+bufsize;
    uint64_t tmp_value;

    *--ptr = '\0';
    do {
        tmp_value = value;
        value /= base;
        *--ptr = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    } while ( value );


    if (tmp_value < 0) *--ptr = '-';
    return ptr;
}
#endif

static char* _itoa10(int64_t value, char* result, size_t bufsize) {
    assert(result);
    char *ptr = result+bufsize;
    int64_t tmp_value;

    *--ptr = '\0';
    do {
        tmp_value = value;
        value /= 10;
        *--ptr = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * 10)];
    } while ( value );


    if (tmp_value < 0) *--ptr = '-';
    return ptr;
}

static size_t printbuf_memappend_fast_n10(struct printbuf *kafka_line_buffer,const uint64_t value){
  assert(kafka_line_buffer);
  static const size_t bufsize = 64;
  char buf[bufsize];

  const char *buf_start = _itoa10(value,buf,bufsize);
  const size_t number_strlen = buf+bufsize-buf_start-1;

  printbuf_memappend_fast(kafka_line_buffer,buf_start,number_strlen);
  return number_strlen;
}

#define get_mac(buffer) net2number(buffer,6);

size_t print_string(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const char *buffer = vbuffer;
  assert_multi(kafka_line_buffer, vbuffer);
  unused_params(flowCache);

  const size_t bef_len = kafka_line_buffer->bpos;
  printbuf_memappend_fast(kafka_line_buffer, buffer + real_field_offset,
                                                                real_field_len);
  return kafka_line_buffer->bpos - bef_len;
}

size_t print_number(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache);

  const uint64_t number = net2number(buffer+real_field_offset,real_field_len);

  return printbuf_memappend_fast_n10(kafka_line_buffer,number);
}

size_t print_netflow_type(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){

    static const char *type_msg = "netflowv";
    const uint8_t *buffer = vbuffer;

    assert_multi(kafka_line_buffer, buffer);
    unused_params(flowCache);

    const size_t start_bpos = kafka_line_buffer->bpos;
    const uint64_t type = net2number(buffer+real_field_offset,real_field_len);
    printbuf_memappend_fast(kafka_line_buffer,type_msg,strlen(type_msg));
    printbuf_memappend_fast_n10(kafka_line_buffer,type);
    return kafka_line_buffer->bpos - start_bpos;
}

/**
 * Append string in print buffer and return increased length
 * @param  kafka_line_buffer Line buffer to append into
 * @param  str               String to append
 * @return                   Increased length
 */
static size_t printbuf_memappend_fast_string(struct printbuf *kafka_line_buffer,const char *str){
  assert_multi(kafka_line_buffer, str);
  const size_t len = strlen(str);
  printbuf_memappend_fast(kafka_line_buffer,str,len);
  return len;
}

size_t print_flow_end_reason(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const char *buffer = vbuffer;
  static const char *reasons[] = {
    [1] = "idle timeout",
    [2] = "active timeout",
    [3] = "end of flow",
    [4] = "forced end",
    [5] = "lack of resources",
  };
  const uint8_t reason = (buffer + real_field_offset)[0];
  assert_multi(kafka_line_buffer, buffer);
  unused_params(real_field_len, flowCache);
  assert(real_field_len > 0);

  if (likely(reason > 0 && reason < sizeof(reasons)/sizeof(reasons[0]))) {
    return printbuf_memappend_fast_string(kafka_line_buffer, reasons[reason]);
  } else {
    traceEvent(TRACE_WARNING,"UNKNOWN flow end reason %d",buffer[0]);
    return 0;
  }
}

size_t print_biflow_direction(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){

  static const char *directions[] = {
    [0] = "arbitrary",
    [1] = "initiator",
    [2] = "reverse initiator",
    [3] = "perimeter",
  };
  const uint8_t *buffer = vbuffer;
  const uint8_t direction = (buffer + real_field_offset)[0];

  assert(real_field_len > 0);
  unused_params(real_field_len, flowCache);

  assert_multi(kafka_line_buffer, buffer);
  if (likely(direction < RD_ARRAYSIZE(directions))) {
    return printbuf_memappend_fast_string(kafka_line_buffer,
                                                        directions[direction]);
  } else {
    traceEvent(TRACE_WARNING,"UNKNOWN buflow direction: %d",buffer[0]);
    return 0;

  }
}

size_t print_direction(struct printbuf *kafka_line_buffer,const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  assert_multi(kafka_line_buffer, flowCache);
  unused_params(buffer, real_field_len, real_field_offset);

  if(flowCache->macs.direction == DIRECTION_UNSET){
    /* Sorry, can't do nothing */
    return 0;
  }

  if(flowCache->macs.direction == DIRECTION_INGRESS)
    return printbuf_memappend_fast_string(kafka_line_buffer,"ingress");
  else if(flowCache->macs.direction == DIRECTION_EGRESS)
    return printbuf_memappend_fast_string(kafka_line_buffer,"egress");
  else if(flowCache->macs.direction == DIRECTION_INTERNAL)
    return printbuf_memappend_fast_string(kafka_line_buffer,"internal");

  if(readOnlyGlobals.enable_debug)
    traceEvent(TRACE_ERROR,"UNKNOWN direction: %d",flowCache->macs.direction);
  return 0;
}

size_t save_direction(struct printbuf *kafka_line_buffer, const void *vbuffer,
    const size_t real_field_len, const size_t real_field_offset,
    struct flowCache *flowCache) {
  const uint8_t *buffer = vbuffer;
  assert_multi(buffer, flowCache);
  assert(real_field_len > 0);
  unused_params(real_field_len, kafka_line_buffer);


  switch((buffer+real_field_offset)[0]){
  case NETFLOW_DIRECTION_INGRESS:
    flowCache->macs.direction = DIRECTION_INGRESS;
    break;
  case NETFLOW_DIRECTION_EGRESS:
    flowCache->macs.direction = DIRECTION_EGRESS;
    break;
  default:
    break; /* Don't know what to do */
  }
  return 0; /* nothing printed */
}

/**
 * Save MAC address in given buffer
 * @param dst_buffer          Destination buffer (have to be sizeof()>6)
 * @param src_buffer_mac_name MAC name description for errors
 * @param kafka_line_buffer   unused
 * @param vbuffer             Buffer where MAC address is
 * @param real_field_len      Length of buffer (needs to be 6)
 * @param real_field_offset   Offset of mac in buffer
 * @param flowCache           unused
 */
static void save_mac(uint8_t *dst_buffer, const char *src_buffer_mac_name,
    struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {

  const uint8_t *buffer = vbuffer;
  assert_multi(buffer);
  unused_params(flowCache, kafka_line_buffer);

  if (likely(real_field_len == 6)) {
    memcpy(dst_buffer, buffer + real_field_offset, 6);
  } else {
    traceEvent(TRACE_WARNING, "%s length != 6.", src_buffer_mac_name);
  }
}

size_t save_src_mac(struct printbuf *kafka_line_buffer,const void *buffer,
    const size_t real_field_len, const size_t real_field_offset, struct flowCache *flowCache){
  unused_params(kafka_line_buffer);

  save_mac(flowCache->macs.src_mac, "Source mac", kafka_line_buffer, buffer,
    real_field_len, real_field_offset, flowCache);
  return 0;
}

size_t save_post_src_mac(struct printbuf *kafka_line_buffer, const void *buffer,
    const size_t real_field_len, const size_t real_field_offset,
    struct flowCache *flowCache){
  save_mac(flowCache->macs.post_src_mac, "PST Source mac", kafka_line_buffer,
    buffer, real_field_len, real_field_offset, flowCache);
  return 0;
}

size_t save_dst_mac(struct printbuf *kafka_line_buffer,const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  assert(flowCache);
  save_mac(flowCache->macs.dst_mac, "DST mac", kafka_line_buffer, buffer,
    real_field_len, real_field_offset, flowCache);
  return 0;
}

size_t save_post_dst_mac(struct printbuf *kafka_line_buffer,const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  save_mac(flowCache->macs.post_dst_mac, "POST DST mac", kafka_line_buffer, buffer,
    real_field_len, real_field_offset, flowCache);
  return 0;
}

static size_t print_ssid_name0(struct printbuf *kafka_line_buffer,const void *buffer,const uint16_t real_field_len){
  const size_t len = strnlen(buffer,real_field_len);
  printbuf_memappend_fast(kafka_line_buffer,buffer,len);
  return len;
}

size_t print_ssid_name(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache);


  return print_ssid_name0(kafka_line_buffer,buffer + real_field_offset,
    real_field_len);
}

/** Transforms ipv4 buffer in ipv6
  @param vbuffer Buffer of ipv6
  @param ipv6 Buffer to sabe IP
  */
static void ipv4buf_to_6(uint8_t ipv6[16],const void *vbuffer){
  const uint8_t *buffer = vbuffer;
  assert_multi(ipv6, buffer);
  int i;
  for (i = 0; i < 10; i++)
    ipv6[i] = 0;

  ipv6[10] = 0xFF;
  ipv6[11] = 0xFF;
  ipv6[12] = buffer[0];
  ipv6[13] = buffer[1];
  ipv6[14] = buffer[2];
  ipv6[15] = buffer[3];
}

/** Prints net information
  @param kafka_line_buffer Buffer to print net information
  @param vbuffer Buffer where net is
  @param real_field_len Length of buffer
  @param real_field_offset Offset of net in buffer
  @param flowCache Flow cache information
  @param sensor_list_search_cb Callback to search in sensor information
  @param global_net_list_cb Callback to manage global nets list.
  @return Printed length
 */

static size_t print_net0(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache,
    const char *(*sensor_list_search_cb)(struct sensor *,const uint8_t[16]),
    const char *(*global_net_list_cb)(const IPNameAssoc *)) {
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer, flowCache);

  if (unlikely(16 != real_field_len)) {
    traceEvent(TRACE_ERROR, "IPv6 net length %zu != 16", real_field_len);
    return 0;
  }

  /* First try: Has the sensor a home net that contains this ip? */
  const char *sensor_home_net = sensor_list_search_cb(flowCache->sensor,
                                                    buffer + real_field_offset);
  if(sensor_home_net){
    return printbuf_memappend_fast_string(kafka_line_buffer, sensor_home_net);
  }

  /* Second try: General nets ip list */
  const IPNameAssoc *ip_name_as = ipInList(buffer + real_field_offset,
    readOnlyGlobals.rb_databases.nets_name_as_list);

  if (ip_name_as) {
    const char *to_print = global_net_list_cb(ip_name_as);
    return printbuf_memappend_fast_string(kafka_line_buffer, to_print);
  } else {
    /* Nothing more to do, sorry */
    return 0;
  }
}

static const char *global_net_list_number(const IPNameAssoc *assoc) {
  return assoc->number;
}

static const char *global_net_list_name(const IPNameAssoc *assoc) {
  return assoc->name;
}

size_t print_net_v6(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  return print_net0(kafka_line_buffer, vbuffer, real_field_len,
    real_field_offset, flowCache, network_ip, global_net_list_number);
}

size_t print_net_name_v6(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  return print_net0(kafka_line_buffer, vbuffer, real_field_len,
    real_field_offset, flowCache, network_name, global_net_list_name);
}

/** Decorate a function making an ipv6 function be called over an ipv4 buffer
 * @param  kafka_line_buffer Kafka line buffer to print
 * @param  vbuffer           Buffer
 * @param  real_field_len    IPv4 length length
 * @param  real_field_offset IPv4 offset in buffer
 * @param  flowCache         Flow cache
 * @param  cb                Function to call with IPv6 buffer
 * @return                   Printed length
 */
static size_t ipv4_to_6_decorator(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache,
    size_t (cb)(struct printbuf *, const void *, const size_t, const size_t,
                                                          struct flowCache *)) {
  const uint8_t *buffer = vbuffer;
  assert(buffer);
  uint8_t ipv6[16];
  if (unlikely(4 != real_field_len)) {
    traceEvent(TRACE_WARNING, "Net length %zu != 4", real_field_len);
    return 0;
  }

  ipv4buf_to_6(ipv6, buffer + real_field_offset);
  return cb(kafka_line_buffer, ipv6, sizeof(ipv6), 0, flowCache);
}

size_t print_net(struct printbuf *kafka_line_buffer,
    const void *buffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  return ipv4_to_6_decorator(kafka_line_buffer, buffer, real_field_len,
            real_field_offset, flowCache, print_net_v6);
}

size_t print_net_name(struct printbuf *kafka_line_buffer,
    const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  return ipv4_to_6_decorator(kafka_line_buffer, buffer, real_field_len,
            real_field_offset, flowCache, print_net_name_v6);
}

static size_t print_ipv4_addr0(struct printbuf *kafka_line_buffer,const uint32_t ipv4){
  assert(kafka_line_buffer);
  static const size_t bufsize = sizeof("255.255.255.255")+1;
  char buf[bufsize];

  const char *ip_as_text = _intoaV4(ipv4,buf,bufsize);
  const size_t ip_as_text_size = buf+bufsize-ip_as_text-1;

  printbuf_memappend_fast(kafka_line_buffer,ip_as_text,ip_as_text_size);
  return ip_as_text_size;
}

static size_t print_ipv4_addr(void *dst_buffer,
    struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  const char *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer, flowCache);

  if (unlikely(4 != real_field_len)) {
    traceEvent(TRACE_ERROR, "IPv4 real field len %zu != 4", real_field_len);
    return 0;
  }

  ipv4buf_to_6(dst_buffer, buffer + real_field_offset);
  const uint32_t ipv4 = net2number(buffer + real_field_offset,4);
  return print_ipv4_addr0(kafka_line_buffer,ipv4);
}

size_t print_ipv4_src_addr(struct printbuf *kafka_line_buffer,
    const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){

  return print_ipv4_addr(flowCache->address.src, kafka_line_buffer, buffer,
    real_field_len, real_field_offset, flowCache);
}

size_t print_ipv4_dst_addr(struct printbuf *kafka_line_buffer,
    const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){

  return print_ipv4_addr(flowCache->address.dst, kafka_line_buffer, buffer,
    real_field_len, real_field_offset, flowCache);
}

static size_t print_mac0(struct printbuf *kafka_line_buffer,
    const void *vbuffer) {
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  int i;
  for(i=0;i<6;++i){
    printbuf_memappend_fast_n16(kafka_line_buffer,buffer[i]);
    if(i<5)
      printbuf_memappend_fast(kafka_line_buffer,":",1);
  }

  return strlen("ff:ff:ff:ff:ff:ff");
}

size_t print_mac(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  const char *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache);

  if (unlikely(real_field_len!=6)) {
    traceEvent(TRACE_ERROR,"Mac with len %zu != 6", real_field_len);
    return 0;
  }

  return print_mac0(kafka_line_buffer, buffer + real_field_offset);
}

static size_t print_mac_vendor0(struct printbuf *kafka_line_buffer,const void *buffer){
  const uint64_t mac = get_mac(buffer);

  if(mac){
    const char *vendor = NULL;
    pthread_rwlock_rdlock(&readOnlyGlobals.rb_databases.mutex);
    if(readOnlyGlobals.rb_databases.mac_vendor_database)
      vendor = rb_find_mac_vendor(mac,readOnlyGlobals.rb_databases.mac_vendor_database);
    pthread_rwlock_unlock(&readOnlyGlobals.rb_databases.mutex);
    if(vendor){
      const size_t vendor_len = strlen(vendor);
      printbuf_memappend_fast(kafka_line_buffer,vendor,vendor_len);
      return vendor_len;
    }
  }

  return 0;
}

static int empty_mac(const uint8_t *mac) {
  static const uint8_t _empty_mac[] = {0,0,0,0,0,0};
  return NULL == mac || 0==memcmp(_empty_mac,mac,6);
}

static const uint8_t *get_direction_based_client_mac(struct flowCache *flowCache){
  assert(flowCache);

  const uint8_t *mac = NULL;
  const uint8_t *dst_mac = NULL;

  switch(is_span_sensor(flowCache->sensor)){
  default:
    traceEvent(TRACE_ERROR,"Unknown sensor span mode %d. Assuming no span.",is_span_sensor(flowCache->sensor));
  case NO_SPAN_MODE:
    dst_mac = flowCache->macs.post_dst_mac;
    break;
  case SPAN_MODE:
    dst_mac = flowCache->macs.dst_mac;
    break;
  }

  /*
    Netflow probe point of view

    (client) -> (probe) traffic => ingress -> client_mac is src mac
    (client) <- (probe) traffic => egress  -> client_mac is dst mac
  */

  if(flowCache->macs.direction == DIRECTION_INGRESS && !empty_mac(flowCache->macs.src_mac)) {
    mac = flowCache->macs.src_mac;
  } else if(flowCache->macs.direction == DIRECTION_EGRESS && !empty_mac(dst_mac)) {
    mac = dst_mac;
  } else if(flowCache->macs.direction == DIRECTION_INTERNAL) {
    /// @TODO test this case
    if(!empty_mac(dst_mac)) {
      mac = dst_mac;
    } else if(!empty_mac(flowCache->macs.src_mac)) {
      mac = flowCache->macs.src_mac;
    }
  }

  return mac;
}

size_t print_direction_based_client_mac(struct printbuf *kafka_line_buffer,const void *buffer,const size_t real_field_len,
  const size_t real_field_offset, struct flowCache *flowCache) {
  unused_params(buffer, real_field_len, real_field_offset);
  assert_multi(kafka_line_buffer, flowCache);

  if (flowCache->client_mac != 0) {
    /* Already printed */
    return 0;
  }

  const uint8_t *mac = get_direction_based_client_mac(flowCache);
  if(mac!=NULL){
    flowCache->client_mac = net2number(mac, 6);
    return print_mac0(kafka_line_buffer, mac);
  }
  return 0;
}

size_t print_client_mac(struct printbuf *kafka_line_buffer,const void *vbuffer,const size_t real_field_len,
  const size_t real_field_offset, struct flowCache *flowCache) {
  static const size_t mac_length = 6;
  const char *buffer = vbuffer;

  if (unlikely(real_field_len != mac_length)) {
    traceEvent(TRACE_ERROR, "Length %zu, Expected %zu", real_field_len,
      mac_length);
    return 0;
  }

  if(flowCache->client_mac != 0) {
    /* Already printed */
    return 0;
  }

  flowCache->client_mac = net2number(buffer + real_field_offset, mac_length);
  return print_mac0(kafka_line_buffer,buffer);
}

size_t print_direction_based_client_mac_vendor(struct printbuf *kafka_line_buffer,const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  unused_params(buffer, real_field_len, real_field_offset);

  const uint8_t *mac = get_direction_based_client_mac(flowCache);
  if(mac!=NULL)
    return print_mac_vendor0(kafka_line_buffer, mac);
  return 0;
}

/* try to print vendor:xx:xx:xx */
static size_t print_mac_vendor_addr_format0(struct printbuf *kafka_line_buffer,
    const void *vbuffer) {
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  const size_t vendor_bytes_written = print_mac_vendor0(kafka_line_buffer,buffer);
  if(vendor_bytes_written>0){
    int i;
    for(i=3;i<6;++i){
      printbuf_memappend_fast(kafka_line_buffer,":",1);
      printbuf_memappend_fast_n16(kafka_line_buffer,buffer[i]);
    }
    return vendor_bytes_written + strlen(":ff:ff:ff");
  }else{
    return 0;
  }
}

static size_t print_mac_map0(struct printbuf *kafka_line_buffer,const void *buffer){
  const uint64_t mac = get_mac(buffer);
  if(mac){
    pthread_rwlock_rdlock(&readOnlyGlobals.rb_databases.mutex);
    const char *char_map = find_mac_name(mac,&readOnlyGlobals.rb_databases.mac_name_database);
    pthread_rwlock_unlock(&readOnlyGlobals.rb_databases.mutex);
    if(char_map){
      printbuf_memappend_fast_string(kafka_line_buffer,char_map);
    }else{
      const size_t bytes_written = print_mac_vendor_addr_format0(kafka_line_buffer,buffer);
      if(bytes_written>0)
        return bytes_written;
    }
  }

  return print_mac0(kafka_line_buffer,buffer);
}

size_t print_mac_map(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache);
  if (unlikely(real_field_len!=6)) {
    traceEvent(TRACE_ERROR,"Mac with real_field_len!=6");
    return 0;
  }

  return print_mac_map0(kafka_line_buffer, buffer + real_field_offset);
}

size_t print_mac_vendor(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache);
  if(real_field_len!=6){
    traceEvent(TRACE_ERROR,"Mac with real_field_len!=6");
    return 0;
  }

  return print_mac_vendor0(kafka_line_buffer, buffer + real_field_offset);
}

static size_t print_engine_id0(struct printbuf *kafka_line_buffer,const uint8_t engine_id){
  assert(kafka_line_buffer);

  if(engine_id == 0)
    return 0;

  return printbuf_memappend_fast_n10(kafka_line_buffer,engine_id);
}

static size_t print_engine_id_name0(struct printbuf *kafka_line_buffer,const uint8_t engine_id){
  assert(kafka_line_buffer);

  if(engine_id == 0)
    return 0;

  pthread_rwlock_rdlock(&readOnlyGlobals.rb_databases.mutex);
  // @TODO change it to an array!
  const NumNameAssoc * node =  numInList(engine_id,readOnlyGlobals.rb_databases.engines_name_as_list);
  pthread_rwlock_unlock(&readOnlyGlobals.rb_databases.mutex);

  if(node){
    return printbuf_memappend_fast_string(kafka_line_buffer,node->name);
  }else{
    return print_engine_id0(kafka_line_buffer,engine_id);
  }
}

size_t print_engine_id(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache, real_field_len);

  return print_engine_id0(kafka_line_buffer,buffer[real_field_offset]);
}

size_t print_engine_id_name(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(real_field_len, flowCache);


  const uint8_t engine_id = net2number(buffer + real_field_offset,1);
  return print_engine_id_name0(kafka_line_buffer,engine_id);
}

static size_t print_application_id0(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len) {
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  if (unlikely(4 != real_field_len)) {
    traceEvent(TRACE_ERROR, "APP_ID length %zu != 4", real_field_len);
    return 0;
  }

  const uint8_t  major_proto = buffer[0];
  const uint32_t minor_proto = net2number(buffer,4) & 0x00FFFFFF;

  if(major_proto == 0 && minor_proto == 0)
    return 0;

  size_t bytes_printed = 0;
  bytes_printed += printbuf_memappend_fast_n10(kafka_line_buffer,major_proto);
  printbuf_memappend_fast(kafka_line_buffer,":",1);
  bytes_printed += 1;
  bytes_printed += printbuf_memappend_fast_n10(kafka_line_buffer,minor_proto);
  return bytes_printed;
}

static size_t print_application_id_name0(struct printbuf *kafka_line_buffer,
    const void *buffer, const size_t real_field_len){
  char err[1024];

  if (NULL==readOnlyGlobals.rb_databases.apps_name_as_list) {
    return 0; /* Nothing to do */
  }

  if (unlikely(4 != real_field_len)) {
    traceEvent(TRACE_ERROR, "APP_ID length %zu != 4", real_field_len);
    return 0;
  }
  const uint32_t appid = net2number(buffer,real_field_len);

  if (appid == 0) {
    return 0;
  }

  const char *appid_str=searchNameAssociatedInTree(readOnlyGlobals.rb_databases.apps_name_as_list,appid,err,sizeof(err));

  if(appid_str){
    return printbuf_memappend_fast_string(kafka_line_buffer,appid_str);
  }else{
    return print_application_id0(kafka_line_buffer,buffer,real_field_len);
  }
}

size_t print_application_id(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const char *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache);
  return print_application_id0(kafka_line_buffer, buffer + real_field_offset,
    real_field_len);
}

size_t print_application_id_name(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const char *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  unused_params(flowCache);
  return print_application_id_name0(kafka_line_buffer,
    buffer + real_field_offset, real_field_len);
}

static size_t print_port0(struct printbuf *kafka_line_buffer,const uint16_t port){
  return printbuf_memappend_fast_n10(kafka_line_buffer,port);
}

//
static size_t save_and_print_port(uint16_t *save_port, const char *port_type,
    struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer, flowCache);

  if (unlikely(real_field_len != 2)) {
    traceEvent(TRACE_ERROR, "%s with len %zu != 2", port_type, real_field_len);
    return 0;
  }

  const uint16_t port = net2number(buffer + real_field_offset, real_field_len);
  *save_port = port;
  return print_port0(kafka_line_buffer,port);
}

size_t print_src_port(struct printbuf *kafka_line_buffer,
    const void *buffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {

  return save_and_print_port(&flowCache->ports.src, "SRC", kafka_line_buffer,
    buffer, real_field_len, real_field_offset, flowCache);
}

size_t print_dst_port(struct printbuf *kafka_line_buffer,
    const void *buffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){

  return save_and_print_port(&flowCache->ports.dst, "DST", kafka_line_buffer,
    buffer, real_field_len, real_field_offset, flowCache);
}

size_t print_srv_port(struct printbuf *kafka_line_buffer,
    const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  assert_multi(kafka_line_buffer);
  unused_params(buffer, real_field_len, real_field_offset);

  if(flowCache->ports.src && flowCache->ports.dst)
    return print_port0(kafka_line_buffer,min(flowCache->ports.src,flowCache->ports.dst));
  else
    return 0;

}

/** Print and store ipv6
 * @param  dst_buf           Destination buffer to save ipv6
 * @param  kafka_line_buffer buffer to print ipv6
 * @param  vbuffer           IPv6 Buffer
 * @param  real_field_len    IPv6 length in bytes
 * @param  real_field_offset IPv6 offset in buffer
 * @param  flowCache         Flow cache
 * @return                   Bytes written in kafka_line_buffer
 */
static size_t print_ipv6(void *vdst_buf, struct printbuf *kafka_line_buffer,
      const void *vbuffer, const size_t real_field_len,
      const size_t real_field_offset) {

  const uint8_t *buffer = (const uint8_t *)vbuffer + real_field_offset;
  uint8_t *dst_buf = vdst_buf;
  size_t i;
  assert_multi(vdst_buf, kafka_line_buffer, buffer);

  if (unlikely(real_field_len != 16)) {
    traceEvent(TRACE_ERROR,"IPv6 field len is not 16");
    return 0;
  }

  // @TODO memcpy(dst_buf, buffer);
  for(i=0;i<16;++i){
    dst_buf[i] = buffer[i];
  }

  for (i=0;i<8;++i) {
    printbuf_memappend_fast_n16(kafka_line_buffer,buffer[2*i]);
    printbuf_memappend_fast_n16(kafka_line_buffer,buffer[2*i+1]);
    if(i<7)
      printbuf_memappend_fast(kafka_line_buffer,":",1);
  }

  return strlen("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");
}

size_t print_ipv6_src_addr(struct printbuf *kafka_line_buffer,
      const void *buffer, const size_t real_field_len,
      const size_t real_field_offset, struct flowCache *flowCache) {
  assert_multi(flowCache);

  return print_ipv6(flowCache->address.src, kafka_line_buffer, buffer,
    real_field_len, real_field_offset);
}

size_t print_ipv6_dst_addr(struct printbuf *kafka_line_buffer,
    const void *buffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  assert_multi(flowCache);

  return print_ipv6(flowCache->address.dst, kafka_line_buffer, buffer,
    real_field_len, real_field_offset);
}

#ifdef HAVE_GEOIP

static size_t append_and_change_quotes(struct printbuf *kafka_line_buffer,
                                                          const void *vbuffer) {
  const char *buffer = vbuffer;
  assert_multi(buffer, vbuffer);
  int i=0;
  while(buffer[i]!=0){
    if(buffer[i] == '\"')
      printbuf_memappend_fast(kafka_line_buffer,"\'",1);
    else
      printbuf_memappend_fast(kafka_line_buffer,&buffer[i],1);
    ++i;
  }

  return i;
}

size_t print_country_code(struct printbuf *kafka_line_buffer,
    const void *buffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){

  assert(buffer);
  unused_params(real_field_offset, flowCache);

  if (unlikely(real_field_len!=4)) {
    traceEvent(TRACE_ERROR,"IP length %zu != 4 bytes.", real_field_len);
    return 0;
  }

  const uint32_t ipv4 = net2number(buffer, 4);
  if (readOnlyGlobals.geo_ip_country_db) {
    pthread_rwlock_rdlock(&readWriteGlobals->geoipRwLock);
    const char * country = GeoIP_country_code_by_ipnum(readOnlyGlobals.geo_ip_country_db,ipv4);
    // const char * country = GeoIP_country_name_by_ipnum(readOnlyGlobals.geo_ip_country_db,ipv4);
    pthread_rwlock_unlock(&readWriteGlobals->geoipRwLock);
    if(country)
      return append_and_change_quotes(kafka_line_buffer,country);
  }

  return 0;
}

struct AS_info{
  const char *number;
  size_t number_len;
  const char *name;
};

static struct AS_info extract_as_from_geoip_response(char *rsp)
{
  /* rsp = ASDDDDD SSSSSSS */
  char *aux = NULL;
  struct AS_info asinfo = {NULL,0,NULL};
  asinfo.number = strtok_r(rsp," ",&aux);
  if(asinfo.number)
  {
    asinfo.number+=2;
    asinfo.number_len = aux-rsp-3;
    asinfo.name = aux;
  }

  return asinfo;
}

size_t print_AS_ipv4(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){

  const uint8_t *buffer = vbuffer;
  assert(buffer);
  unused_params(flowCache);

  if (unlikely(4 != real_field_len)) {
    traceEvent(TRACE_ERROR, "IPv4 length %zu != 4", real_field_len);
    return 0;
  }

  const unsigned long ipv4 = net2number(buffer + real_field_offset, 4);
  size_t written_len = 0;

  if(ipv4){
    char *rsp=NULL;
    pthread_rwlock_rdlock(&readWriteGlobals->geoipRwLock);
    if(readOnlyGlobals.geo_ip_asn_db)
      rsp = GeoIP_name_by_ipnum(readOnlyGlobals.geo_ip_asn_db, ipv4);
    pthread_rwlock_unlock(&readWriteGlobals->geoipRwLock);

    if(rsp){
      struct AS_info asinfo = extract_as_from_geoip_response(rsp);
      if(asinfo.number){
        printbuf_memappend_fast(kafka_line_buffer,asinfo.number,asinfo.number_len);
        written_len = asinfo.number_len;
      }
      free(rsp);
    }
  }

  return written_len;
}

static size_t print_buffer_geoip_AS_name(struct printbuf *kafka_line_buffer,char *rsp) {
  size_t written_len = 0;

  if(NULL == rsp)
    return 0;

  char *toprint = strchr(rsp,' ');
  if(toprint && *(toprint+1)!='\0')
    written_len = append_escaped(kafka_line_buffer,toprint+1,strlen(toprint+1));
  free(rsp);

  return written_len;
}

static size_t print_AS_ipv4_name0(struct printbuf *kafka_line_buffer, const void *buffer, const uint16_t real_field_len){
  assert_multi(kafka_line_buffer, buffer);

  const uint64_t ipv4 = net2number(buffer,real_field_len);
  size_t written_len = 0;

  char * rsp=NULL;
  if(readOnlyGlobals.geo_ip_asn_db){
    pthread_rwlock_rdlock(&readWriteGlobals->geoipRwLock);
    rsp = GeoIP_name_by_ipnum(readOnlyGlobals.geo_ip_asn_db, ipv4);
    pthread_rwlock_unlock(&readWriteGlobals->geoipRwLock);
  }

  if(rsp){
    written_len = print_buffer_geoip_AS_name(kafka_line_buffer,rsp);
  }

  return written_len;
}

size_t print_AS_ipv4_name(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert(buffer);
  (void)flowCache;

  if (likely(real_field_len==4)) {
    return print_AS_ipv4_name0(kafka_line_buffer, buffer + real_field_offset,
                                                                real_field_len);
  } else {
    traceEvent(TRACE_ERROR,"IPv4 with len %zu != 4.", real_field_len);
    return 0;
  }
}

#define IPV6_LEN 16
static struct in6_addr get_ipv6(const uint8_t *buffer){
  struct in6_addr ipv6;
  memcpy(&ipv6.s6_addr,buffer,IPV6_LEN);
  return ipv6;
}

static size_t print_AS6_name0(struct printbuf *kafka_line_buffer,const struct in6_addr *ipv6){
  assert(kafka_line_buffer);
  assert(ipv6);

  char * rsp=NULL;
  size_t written_len = 0;
  if(readOnlyGlobals.geo_ip_asn_db_v6){
    pthread_rwlock_rdlock(&readWriteGlobals->geoipRwLock);
    rsp = GeoIP_name_by_ipnum_v6(readOnlyGlobals.geo_ip_asn_db_v6, *ipv6);
    pthread_rwlock_unlock(&readWriteGlobals->geoipRwLock);
  }

  if(rsp){
    written_len = print_buffer_geoip_AS_name(kafka_line_buffer,rsp);
  }

  return written_len;
}

size_t print_AS6_name(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache){
  const uint8_t *buffer = vbuffer;
  assert(buffer);
  (void)flowCache;

  if (unlikely(real_field_len!=16)) {
    traceEvent(TRACE_ERROR,"IPv6 length %zu != 16.", real_field_len);
    return 0;
  }

  const struct in6_addr ipv6 = get_ipv6(buffer + real_field_offset);
  return print_AS6_name0(kafka_line_buffer,&ipv6);
}

static size_t print_AS6_0(struct printbuf *kafka_line_buffer,const struct in6_addr *ipv6){
  assert(ipv6);

  char *rsp=NULL;
  size_t bytes_printed = 0;
  if(readOnlyGlobals.geo_ip_asn_db_v6){
    pthread_rwlock_rdlock(&readWriteGlobals->geoipRwLock);
    rsp = GeoIP_name_by_ipnum_v6(readOnlyGlobals.geo_ip_asn_db_v6, *ipv6);
    pthread_rwlock_unlock(&readWriteGlobals->geoipRwLock);
  }

  if(rsp){
    struct AS_info asinfo = extract_as_from_geoip_response(rsp);
    if(asinfo.number){
      printbuf_memappend_fast(kafka_line_buffer,asinfo.number,asinfo.number_len);
      bytes_printed = asinfo.number_len;
    }
    free(rsp);
  }

  return bytes_printed;
}

size_t print_AS6(struct printbuf *kafka_line_buffer,
    const void *vbuffer,const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  const uint8_t *buffer = vbuffer;
  (void)flowCache;

  if (unlikely(real_field_len!=16)) {
    traceEvent(TRACE_ERROR,"IPv6 length %zu != 16.", real_field_len);
    return 0;
  }

  const struct in6_addr ipv6 = get_ipv6(buffer + real_field_offset);
  return print_AS6_0(kafka_line_buffer,&ipv6);
}

static size_t print_country6_code0(struct printbuf *kafka_line_buffer, const struct in6_addr *ipv6){
  if(readOnlyGlobals.geo_ip_country_db_v6){
    pthread_rwlock_rdlock(&readWriteGlobals->geoipRwLock);
    const char * country = GeoIP_country_code_by_ipnum_v6(readOnlyGlobals.geo_ip_country_db_v6,*ipv6);
    pthread_rwlock_unlock(&readWriteGlobals->geoipRwLock);
    if(country)
      return append_and_change_quotes(kafka_line_buffer,country);
  }
  return 0;
}

size_t print_country6_code(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  (void)flowCache;

  if(unlikely(real_field_len!=16)){
    traceEvent(TRACE_ERROR,"IPv6 length != 16.");
    return 0;
  }

  const struct in6_addr ipv6 = get_ipv6(buffer + real_field_offset);
  return print_country6_code0(kafka_line_buffer,&ipv6);
}

#endif /* HAVE_GEOIP */

size_t print_proto_name(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache) {
  const uint8_t *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  (void)flowCache;

  if (unlikely(real_field_len!=2)) {
    traceEvent(TRACE_ERROR,"protocol length %zu != 2.", real_field_len);
    return 0;
  }

  const uint16_t proto = net2number(buffer + real_field_offset, 2);

  const char *proto_map = proto2name(proto);
  if (proto_map) {
    return printbuf_memappend_fast_string(kafka_line_buffer, proto_map);
  } else {
    return printbuf_memappend_fast_n10(kafka_line_buffer, proto);
  }
}

size_t print_sensor_enrichment(struct printbuf *kafka_line_buffer,
    const struct flowCache *flowCache)
{
  if(!flowCache){
    traceEvent(TRACE_ERROR,"Not flowCache given");
    return 0;
  }

  if(!flowCache->sensor){
    traceEvent(TRACE_ERROR,"Not flowCache->sensor given");
    return 0;
  }

  const char *enrichment = sensor_ip_enrichment(flowCache->sensor);
  if(enrichment && enrichment[0] != '\0') {
    size_t added = 0;
    added += printbuf_memappend_fast_string(kafka_line_buffer,",");
    added += printbuf_memappend_fast_string(kafka_line_buffer,enrichment);
    return added;
  } else {
    return 0;
  }
}

#ifdef HAVE_CISCO_URL

static size_t print_cisco_private_buffer(struct printbuf *kafka_line_buffer,
    const void *vbuffer, const size_t real_field_len,
    const size_t real_field_offset, struct flowCache *flowCache,
    const char *expected_identifier, size_t expected_identifier_length) {
  const char *buffer = vbuffer;
  assert_multi(kafka_line_buffer, buffer);
  (void)flowCache;

  if(real_field_len <= expected_identifier_length) {
    return 0; /* nothing to do */
  }

  if(0!=memcmp(expected_identifier,&buffer[real_field_offset],
                                     expected_identifier_length)) {
    return 0;
  }

  return append_escaped(kafka_line_buffer,buffer+real_field_offset+6,
                                                        real_field_len-6);

}

size_t print_http_url(struct printbuf *kafka_line_buffer,
  const void *buffer,const size_t real_field_len,
  const size_t real_field_offset,struct flowCache *flowCache) {

  static const char http_url_id[] = {0x03, 0x00, 0x00, 0x50, 0x34, 0x01};
  return print_cisco_private_buffer(kafka_line_buffer,
    buffer,real_field_len,real_field_offset,flowCache,
    http_url_id,sizeof(http_url_id));
}

size_t print_http_host(struct printbuf *kafka_line_buffer,
  const void *buffer,const size_t real_field_len,
  const size_t real_field_offset,struct flowCache *flowCache) {

  static const char http_host_id[] = {0x03, 0x00, 0x00, 0x50, 0x34, 0x02};
  return print_cisco_private_buffer(kafka_line_buffer,
    buffer,real_field_len,real_field_offset,flowCache,
    http_host_id,sizeof(http_host_id));
}

size_t print_http_user_agent(struct printbuf *kafka_line_buffer,
  const void *buffer,const size_t real_field_len,
  const size_t real_field_offset,struct flowCache *flowCache) {

  static const char http_ua_id[] = {0x03, 0x00, 0x00, 0x50, 0x34, 0x03};
  return print_cisco_private_buffer(kafka_line_buffer,
    buffer,real_field_len,real_field_offset,flowCache,
    http_ua_id,sizeof(http_ua_id));
}

size_t print_http_referer(struct printbuf *kafka_line_buffer,
  const void *buffer,const size_t real_field_len,
  const size_t real_field_offset,struct flowCache *flowCache) {

  static const char http_referer_id[] = {0x03, 0x00, 0x00, 0x50, 0x34, 0x04};
  return print_cisco_private_buffer(kafka_line_buffer,
    buffer,real_field_len,real_field_offset,flowCache,
    http_referer_id,sizeof(http_referer_id));
}

size_t print_https_common_name(struct printbuf *kafka_line_buffer,
  const void *buffer,const size_t real_field_len,
  const size_t real_field_offset,struct flowCache *flowCache) {

  static const char https_command_name_nbar_id[] = {0x0d, 0x00, 0x01, 0xc5, 0x34, 0x01};
  return print_cisco_private_buffer(kafka_line_buffer,
    buffer,real_field_len,real_field_offset,flowCache,
    https_command_name_nbar_id,sizeof(https_command_name_nbar_id));
}

#ifdef HAVE_UDNS

static size_t print_dns_obtained_hostname(struct printbuf *kafka_line_buffer,
  const char *string) {

  return string ? printbuf_memappend_fast_string(kafka_line_buffer,string) : 0;
}

static const char *get_direction_based_client_hostname(struct flowCache *flowCache) {
  return flowCache->address.client_name ? flowCache->address.client_name :
    flowCache->address.client_name_cache ? flowCache->address.client_name_cache->name : NULL;
}

static const char *get_direction_based_target_hostname(struct flowCache *flowCache) {
  return flowCache->address.target_name ? flowCache->address.target_name :
    flowCache->address.target_name_cache ? flowCache->address.target_name_cache->name : NULL;
}

#ifdef HAVE_UDNS

static const uint8_t *get_src_ip(struct flowCache *flowCache) {
  return (uint8_t *)flowCache->address.src;
}

static const uint8_t *get_dst_ip(struct flowCache *flowCache) {
  return (uint8_t *)flowCache->address.dst;
}

const uint8_t *get_direction_based_client_ip(struct flowCache *flowCache) {
  assert(flowCache);

  const uint8_t *ret = NULL;
  const uint8_t *src_ip = get_src_ip(flowCache);
  const uint8_t *dst_ip = get_dst_ip(flowCache);

  /*
    Netflow probe point of view

    (client) -> (probe) traffic => ingress -> client_mac is src mac
    (client) <- (probe) traffic => egress  -> client_mac is dst mac
  */
  /// @NOTE keep in sync with get_direction_based_client_mac
  if(flowCache->macs.direction == DIRECTION_INGRESS && NULL != src_ip) {
    ret = src_ip;
  } else if(flowCache->macs.direction == DIRECTION_EGRESS && NULL != dst_ip) {
    ret = dst_ip;
  } else if(flowCache->macs.direction == DIRECTION_INTERNAL) {
    if(NULL != dst_ip) {
      ret = dst_ip;
    } else if(NULL != src_ip){
      ret = src_ip;
    }
  }

  return ret;
}

const uint8_t *get_direction_based_target_ip(struct flowCache *flowCache) {
  const uint8_t *src_ip = get_src_ip(flowCache);
  const uint8_t *dst_ip = get_dst_ip(flowCache);

  const uint8_t *client_hostname = get_direction_based_client_ip(flowCache);
  return (client_hostname == src_ip) ?
    dst_ip : src_ip;
}

#endif /* HAVE_UDNS */

/// @TODO difference client/target src/dst
size_t print_client_name(struct printbuf *kafka_line_buffer,
  const void *buffer,const size_t real_field_len,
  const size_t real_field_offset,struct flowCache *flowCache) {

  return print_dns_obtained_hostname(kafka_line_buffer,
    get_direction_based_client_hostname(flowCache));
}

size_t print_target_name(struct printbuf *kafka_line_buffer,
  const void *buffer,const size_t real_field_len,
  const size_t real_field_offset,struct flowCache *flowCache) {

  return print_dns_obtained_hostname(kafka_line_buffer,
    get_direction_based_target_hostname(flowCache));
}

#endif

#endif

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

#ifdef __GNUC__
// For some reason, GCC <= 4.7 does not provide these macros
#if !__GNUC_PREREQ(4,8)
#define __BYTE_ORDER__ __BYTE_ORDER
#define __ORDER_LITTLE_ENDIAN__ __LITTLE_ENDIAN
#define __ORDER_BIG_ENDIAN__ __BIG_ENDIAN
#define __builtin_bswap16(a) (((a)&0xff)<<8u)|((a)>>8u)
#endif // GCC < 4.8
#endif // __GNUC__

#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
#define constexpr_be16toh(x) __builtin_bswap16(x)
#define constexpr_be32toh(x) __builtin_bswap32(x)
#else
#define constexpr_be16toh(x) (x)
#define constexpr_be32toh(x) (x)
#endif

#define ARGS(...) __VA_ARGS__

#define NF5_IP(a, b, c, d) (((a)<<24)|((b)<<16)|((c)<<8)|(d))

// Convert an uint16_t to BIG ENDIAN uint8_t[2] array initializer
#define UINT16_TO_UINT8_ARR(x) ((x)>>8), ((x)&0xff)

#define UINT32_TO_UINT8_ARR(x) \
	UINT16_TO_UINT8_ARR((x)>>16), UINT16_TO_UINT8_ARR((x)&0xffff)

#define UINT64_TO_UINT8_ARR(x) \
	UINT32_TO_UINT8_ARR((x##l)>>32), UINT32_TO_UINT8_ARR((x##l)&0xffffffff)

#define TEMPLATE_ENTITY(entity, len) \
	UINT16_TO_UINT8_ARR(entity), UINT16_TO_UINT8_ARR(len)

#define TEMPLATE_PRIVATE_ENTITY(field_type, len, pen) \
	UINT16_TO_UINT8_ARR(field_type | 0x8000), \
	UINT16_TO_UINT8_ARR(len), UINT32_TO_UINT8_ARR(pen)

#define FLOW_APPLICATION_ID(type, id) UINT32_TO_UINT8_ARR(type<<24 | id)

/* ********************** TEMPLATE & FLOW COMMON STUFF ********************** */
#define BYTE_ARRAY_SIZE(...) sizeof((uint8_t[]){ __VA_ARGS__ })

/* ***************************** TEMPLATE STUFF ***************************** */
#define TEMPLATE_BYTES_0(entity, length, pen) \
				TEMPLATE_ENTITY(entity, length)
#define TEMPLATE_BYTES_9(entity, length, pen) \
				TEMPLATE_PRIVATE_ENTITY(entity, length, pen) \

#define TEMPLATE_BYTES(entity, length, pen, ...) \
				TEMPLATE_BYTES_##pen(entity, length, pen),

#define TEMPLATE_ENTITY_SIZE(entity, length, pen, ...) \
	+BYTE_ARRAY_SIZE(TEMPLATE_BYTES(entity, length, pen, __VA_ARGS__))
#define TEMPLATE_BYTES_LENGTH(ENTITIES) ENTITIES(TEMPLATE_ENTITY_SIZE)

#define R_1(...) +1
#define TEMPLATE_ENTITIES_COUNT(ENTITIES) ENTITIES(R_1)

#define IPFIX_1TEMPLATE(var, FLOW_HEADER, TEMPLATE_ID, ENTITIES) \
struct { \
	IPFIXFlowHeader flowHeader; \
	IPFIXSet flowSetHeader; \
	V9TemplateDef templateHeader; /* It's the same */ \
	uint8_t templateBuffer[TEMPLATE_BYTES_LENGTH(ENTITIES)]; \
} __attribute__((packed)) var = { \
	.flowHeader = { \
		.version = constexpr_be16toh(10), \
		.len = constexpr_be16toh(TEMPLATE_BYTES_LENGTH(ENTITIES) \
			+ sizeof(V9TemplateDef) + sizeof(IPFIXSet) \
			+ sizeof(IPFIXFlowHeader)), \
		FLOW_HEADER \
	}, .flowSetHeader = { \
		/*uint16_t*/ .set_id = constexpr_be16toh(2), \
		/*uint16_t*/ .set_len = constexpr_be16toh( \
			TEMPLATE_BYTES_LENGTH(ENTITIES) \
			+ sizeof(V9TemplateDef) + sizeof(IPFIXSet)), \
	}, .templateHeader = { \
		.templateId = constexpr_be16toh(TEMPLATE_ID), \
		.fieldCount = constexpr_be16toh( \
			TEMPLATE_ENTITIES_COUNT(ENTITIES)), \
	}, .templateBuffer = { ENTITIES(TEMPLATE_BYTES) } \
}

/* ******************************* FLOW STUFF ******************************* */
#define FLOW_BYTES(entity, length, pen, ...) __VA_ARGS__,

#define FLOW_ENTITY_SIZE(entity, length, pen, ...) \
	+BYTE_ARRAY_SIZE(FLOW_BYTES(entity, length, pen, __VA_ARGS__))
#define FLOW_BYTES_LENGTH(ENTITIES) ENTITIES(FLOW_ENTITY_SIZE)
#define IPFIX_1FLOW(var, FLOW_HEADER, TEMPLATE_ID, ENTITIES) \
struct { \
	IPFIXFlowHeader flowHeader; \
	IPFIXSet flowSetHeader; \
	uint8_t buffer1[FLOW_BYTES_LENGTH(ENTITIES)]; \
} __attribute__((packed)) var = { \
	.flowHeader = { \
		.version = constexpr_be16toh(10), \
		.len = constexpr_be16toh(FLOW_BYTES_LENGTH(ENTITIES) \
				+ sizeof(IPFIXSet) + sizeof(IPFIXFlowHeader)), \
		FLOW_HEADER \
	}, .flowSetHeader = { \
		.set_id = constexpr_be16toh(TEMPLATE_ID), \
		.set_len = constexpr_be16toh(FLOW_BYTES_LENGTH(ENTITIES) \
							+ sizeof(IPFIXSet)), \
	}, .buffer1 = {	ENTITIES(FLOW_BYTES) }, \
};

/* ************************************************************************** */
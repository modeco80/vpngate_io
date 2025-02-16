#ifndef VPNGATE_IO_CAPI_VALUE_H
#define VPNGATE_IO_CAPI_VALUE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* the type of a pack value */
typedef enum {
	VPNGATE_IO_VALUETYPE_INT = 0,
	VPNGATE_IO_VALUETYPE_DATA = 1,
	VPNGATE_IO_VALUETYPE_STRING = 2,
	VPNGATE_IO_VALUETYPE_WSTRING = 3,
	VPNGATE_IO_VALUETYPE_INT64 = 4
} vpngate_io_value_type;

/* get the string name of a vpngate_io_value_type */
const char* vpngate_io_value_type_to_string(vpngate_io_value_type type);

/* a loosely typed value */
typedef struct {
	vpngate_io_value_type type;
	union {
		uint32_t intValue;

		struct {
			uint8_t* ptr;
			size_t len;
		} dataValue;

		struct {
			const char* ptr;
			size_t len;
		} stringValue;

		struct {
			const char* ptr;
			size_t len;
		} wstringValue;

		uint64_t int64Value;
	};
} vpngate_io_value;

#ifdef __cplusplus
};
#endif

#endif
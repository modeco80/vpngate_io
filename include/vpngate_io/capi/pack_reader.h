#ifndef VPNGATE_IO_CAPI_PACK_READER_H
#define VPNGATE_IO_CAPI_PACK_READER_H

#include <stddef.h>
#include <stdint.h>

#include <vpngate_io/capi/value.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct _vpngate_io_PackReader vpngate_io_PackReader;

    /* Create a new PackReader over the given bytes.
       They MUST NOT be freed until vpngate_io_pack_reader_free() is called */
    vpngate_io_PackReader* vpngate_io_pack_reader_new(uint8_t* pBuffer, size_t dataSize);

    /* Returns the value type for a key, or VPNGATE_IO_ERRC_KEY_DOES_NOT_EXIST if it does not exist */
    int vpngate_io_pack_reader_value_type(vpngate_io_PackReader* reader, const char* key, vpngate_io_value_type* pOutType);

    /* Returns how many values are in this key, or VPNGATE_IO_ERRC_KEY_DOES_NOT_EXIST  */
    int vpngate_io_pack_reader_length(vpngate_io_PackReader* pack, const char* key, size_t* outLen);

    /* Gets values. 
        Writes to to pValues[0..length]. 
        Preallocate using vpngate_io_pack_reader_length().
    */
    int vpngate_io_pack_reader_get(vpngate_io_PackReader* reader, const char* key, vpngate_io_value* pValues, vpngate_io_value_type type);

    /* Free a PackReader. */
    void vpngate_io_pack_reader_free(vpngate_io_PackReader* reader);

#ifdef __cplusplus
};
#endif

#endif
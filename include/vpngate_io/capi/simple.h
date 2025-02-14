#ifndef VPNGATE_IO_SIMPLE_H
#define VPNGATE_IO_SIMPLE_H

#include <stddef.h>
#include <stdint.h>

#include <vpngate_io/capi/pack_reader.h>

#ifdef __cplusplus
extern "C" {
#endif

    /* The "simple" struct. Currently this is the only way to use vpngate_io in C code,
       but later on bindings to more internal pieces in C++ may be considered for inclusion. */
    typedef struct _vpngate_io_Simple vpngate_io_Simple;

    /* Creates a new simple object. */
    vpngate_io_Simple* vpngate_io_simple_new(const char* path);

    int vpngate_io_simple_init(vpngate_io_Simple* simple);

    /* Gets the pack reader for a simple instance.
        Only call if vpngate_io_simple_init returns OK.

        Also: **DO NOT** try to free this; it will be collected
        properly by C++ code in vpngate_io_simple_free!!!
    */
    vpngate_io_PackReader* vpngate_io_simple_get_pack_reader(vpngate_io_Simple* simple);

    /* Deletes a simple object */
    void vpngate_io_simple_free(vpngate_io_Simple* simple);

#ifdef __cplusplus
};
#endif

#endif
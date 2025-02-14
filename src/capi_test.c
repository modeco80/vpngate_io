// Test to make sure the C API can actually be consumed from C
#include <vpngate_io/capi/error.h>
#include <vpngate_io/capi/simple.h>
#include <vpngate_io/capi/pack_reader.h>

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	vpngate_io_Simple* simple = vpngate_io_simple_new(argv[1]);
    vpngate_io_PackReader* pack;

	int res = vpngate_io_simple_init(simple);
	if(res != VPNGATE_IO_ERRC_OK) {
        printf("Error initalizing simple object: %s\n", vpngate_io_strerror(res));
        // any complaints about using goto can be sent to "Use the C++ API that's already there
        // and much nicer, the C API is for FFI usage mainly" mail box.
        goto cleanup;
    }

    pack = vpngate_io_simple_get_pack_reader(simple);

    size_t nrValues = 0;
    res = vpngate_io_pack_reader_length(pack, "ID", &nrValues);

    if(res != VPNGATE_IO_ERRC_OK) {
        printf("Error getting value count: %s\n", vpngate_io_strerror(res));
        goto cleanup;
    }

    vpngate_io_value_type valueType;
    res = vpngate_io_pack_reader_value_type(pack, "ID", &valueType);

    if(res != VPNGATE_IO_ERRC_OK) {
        printf("Error getting value type: %s\n", vpngate_io_strerror(res));
        goto cleanup;
    }

    printf("Type is %d, with %lu values\n", valueType, nrValues);

    vpngate_io_value* values = (vpngate_io_value*)calloc(nrValues, sizeof(vpngate_io_value));

    res = vpngate_io_pack_reader_get(pack, "ID", &values[0], valueType);

    if(res != VPNGATE_IO_ERRC_OK) {
        printf("Error getting value data: %s\n", vpngate_io_strerror(res));
        goto cleanup;
    }

    // Print all values
    for(size_t i = 0; i < nrValues; ++i) {
        printf("ID[%lu] = %lu\n", i, values[i].int64Value);
    }

    // Clean up after ourselves.
cleanup:
    if(values) {
        free(values);
        values = NULL;
    }

    /* We do not need to manually free the pack reader pointer, since
       it is collected in vpngate_io_simple_free(). */
    pack = NULL;
    if(simple) {
        vpngate_io_simple_free(simple);
        simple = NULL;
    }

    return 0;

}
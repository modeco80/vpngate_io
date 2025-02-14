
#include <vpngate_io/capi/error.h>
extern "C" {

const char* vpngate_io_strerror(int errc) {
	// clang-format off
	switch(errc) {
        case VPNGATE_IO_ERRC_OK: return "No error"; break;
        case VPNGATE_IO_ERRC_KEY_DOES_NOT_EXIST: return "The given key does not exist"; break;
        case VPNGATE_IO_ERRC_INVALID_ARGUMENT: return "An invalid argument was given to a function"; break;
        case VPNGATE_IO_ERRC_INVALID_FILE: return "Simple tried to parse an invalid file"; break;
        case VPNGATE_IO_ERRC_OOB: return "An attempt to read out of bounds memory was caught"; break;
        case VPNGATE_IO_ERRC_TYPE_MISMATCH: return "Mismatched type"; break;
		default: return "Unknown error"; break;
	}
	// clang-format on
}
};
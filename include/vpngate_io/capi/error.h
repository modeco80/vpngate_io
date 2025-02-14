#ifndef VPNGATE_IO_CAPI_ERROR_H
#define VPNGATE_IO_CAPI_ERROR_H

#define VPNGATE_IO_ERRC_OK 0 /* No error has occured */
#define VPNGATE_IO_ERRC_KEY_DOES_NOT_EXIST 1 /* The given key does not exist */
#define VPNGATE_IO_ERRC_INVALID_ARGUMENT 2 /* An invalid argument was provided */
#define VPNGATE_IO_ERRC_INVALID_FILE 3 /* An invalid file was given to simple API functions */
#define VPNGATE_IO_ERRC_OOB 4 /* An attempt to read out of bounds was caught */
#define VPNGATE_IO_ERRC_TYPE_MISMATCH 5 /* A type was mismatched */

#ifdef __cplusplus
extern "C" {
#endif

    /* Returns a descriptive string based on what error occured. */
    const char* vpngate_io_strerror(int errc);

#ifdef __cplusplus
};
#endif

#endif
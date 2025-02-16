
#include <vpngate_io/value_types.hpp>
#include <vpngate_io/capi/value.h>

extern "C" {
    const char* vpngate_io_value_type_to_string(vpngate_io_value_type type) {
        auto ret = vpngate_io::ValueTypeToString(static_cast<vpngate_io::ValueType>(type));
        return ret.data();
    }
}
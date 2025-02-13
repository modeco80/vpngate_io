#include <zlib.h>

#include <cstring>
#include <vpngate_io/dat_file.hpp>
#include <vpngate_io/pack_reader.hpp>

namespace vpngate_io {

	std::unique_ptr<std::uint8_t[]> GetDATPackData(vpngate_io::PackReader& pack, std::size_t& outSize) {
		// Handle the data being packed; this seems to be the default
		// now, but we also can safely handle the data NOT being packed

		std::size_t dataSize = 0;
		bool dataCompressed = false;

		if(auto c = pack.GetFirst<ValueType::Int>("compressed"); c.has_value()) {
			auto compressed = c.value();

			if(compressed == 1) {
				dataCompressed = true;
				dataSize = pack.GetFirst<ValueType::Int>("data_size").value();
			} else {
				dataSize = pack.GetFirst<ValueType::Data>("data").value().size();
			}
		}

		auto dataUnpackBuffer = std::make_unique<std::uint8_t[]>(dataSize);
		auto dataSource = pack.GetFirst<ValueType::Data>("data").value();

		if(dataCompressed) {
			std::size_t size = dataSize;
			auto res = uncompress(dataUnpackBuffer.get(), &size, &dataSource[0], dataSource.size());

			if(res != Z_OK) {
				// TODO: throw here
				return nullptr;
			}
		} else {
			// Just memcpy() it
			memcpy(&dataUnpackBuffer[0], &dataSource[0], dataSource.size());
		}

		outSize = dataSize;
		return dataUnpackBuffer;
	}

} // namespace vpngate_io
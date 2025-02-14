#include <memory>
#include <vpngate_io/pack_reader.hpp>

namespace vpngate_io {

	enum class SimpleErrc : std::uint32_t {
		Ok = 0,
		InvalidDat = 1,
	};

	/// Simple provides a, well.. simple! interface to
	/// vpngate_io. File I/O, and most of the busywork is handled by
	/// this struct, and PackReader() will grant access to the DAT's data
	/// in a relatively easy manner.
	///
	/// You can always go lower level if you want,
	/// but this struct should be usable enough in most cases.
	struct Simple {
		Simple(std::string_view filename);

		/// Does further initalization of this simple.
		SimpleErrc Init();

		vpngate_io::PackReader& PackReader();

		const std::string& GetIdentifier() const;

	   private:
		std::string filename;

		std::unique_ptr<std::uint8_t[]> data;
		std::size_t dataSize;

		std::string identifier;

		std::optional<vpngate_io::PackReader> reader;
	};

} // namespace vpngate_io
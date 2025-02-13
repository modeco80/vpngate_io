#include <stdexcept>
#include <vpngate_io/pack_reader.hpp>

namespace vpngate_io::impl {

	namespace {
		/// Helper to make advancing a buffer pointer safe. Will throw if an attempt to 
		/// put the buffer out of bounds is made.
		void SafeAdvance(std::uint8_t* bufferStart, std::uint8_t*& bufptr, std::size_t advanceCount, std::size_t bufferSize) {
			// TODO: Definitely there's more needed
			if((bufptr - bufferStart) + advanceCount > bufferSize)
				throw std::runtime_error("PackReader: Attempt to exceed bounds of buffer!");

			bufptr += advanceCount;
		}

		void AdvanceToNextValue(std::uint8_t* bufferStart, std::uint8_t*& bufptr, ValueType type, std::size_t bufferSize) {
			switch(type) {
				case ValueType::Int: {
					SafeAdvance(bufferStart, bufptr, 4, bufferSize);
				} break;

				case ValueType::Data: {
					auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
					SafeAdvance(bufferStart, bufptr, 4 + dataSize, bufferSize);
				} break;

				case ValueType::String: {
					auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
					SafeAdvance(bufferStart, bufptr, 4 + dataSize, bufferSize);
				} break;

				case ValueType::WString: {
					auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
					SafeAdvance(bufferStart, bufptr, 4 + dataSize, bufferSize);
				} break;

				case ValueType::Int64: {
					SafeAdvance(bufferStart, bufptr, 8, bufferSize);
				} break;

				default:
					break;
			}
		}
	} // namespace

	void PackReader::WalkAllImpl(void (*Func)(void*, ValueType, std::string_view, std::size_t, std::uint8_t*), void* user) {
		auto* bufptr = buffer;

		// Swap elements
		auto nrElements = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
		SafeAdvance(buffer, bufptr, 4, size);

		for(auto i = 0; i < nrElements; ++i) {
			auto elementNameLength = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			SafeAdvance(buffer, bufptr, 4, size);

			auto elementName = std::string_view(reinterpret_cast<const char*>(bufptr), elementNameLength - 1);
			SafeAdvance(buffer, bufptr, elementNameLength - 1, size);

			auto elementType = static_cast<ValueType>(Swap(*reinterpret_cast<std::uint32_t*>(bufptr)));
			SafeAdvance(buffer, bufptr, 4, size);

			auto elementNumValues = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			SafeAdvance(buffer, bufptr, 4, size);

			// Walk all elements
			for(auto j = 0; j < elementNumValues; ++j) {
				switch(elementType) {
					case ValueType::Int: {
						Func(user, elementType, elementName, 4, bufptr);
					} break;

					case ValueType::Data: {
						auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));

						Func(user, elementType, elementName, dataSize, bufptr + 4);
					} break;

					case ValueType::String: {
						auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
						Func(user, elementType, elementName, dataSize, bufptr + 4);
					} break;

					case ValueType::WString: {
						auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));

						// :((((
						if(dataSize == 0) {
							Func(user, elementType, elementName, 0, nullptr);
						} else {
							Func(user, elementType, elementName, dataSize - 1, bufptr + 4);
						}
					} break;

					case ValueType::Int64: {
						Func(user, elementType, elementName, 8, bufptr);
					} break;

					default:
						break;
				}

				AdvanceToNextValue(buffer, bufptr, elementType, size);
			}
		}
	}

	void PackReader::WalkValuesImpl(std::uint8_t* pValueStart, ValueType type, std::size_t nrValues, void (*func)(void* user, std::size_t, std::size_t, std::uint8_t*), void* user) {
		auto bufptr = pValueStart;

		for(auto j = 0; j < nrValues; ++j) {
			switch(type) {
				case ValueType::Int: {
					func(user, j, 4, bufptr);
				} break;

				case ValueType::Data: {
					auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
					func(user, j, dataSize, bufptr + 4);
				} break;

				case ValueType::String: {
					auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));

					func(user, j, dataSize, bufptr + 4);
				} break;

				case ValueType::WString: {
					auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));

					// :((((
					if(dataSize == 0) {
						func(user, j, 0, nullptr);
					} else {
						func(user, j, dataSize - 1, bufptr + 4);
					}
				} break;

				case ValueType::Int64: {
					func(user, j, 8, bufptr);
				} break;

				default:
					break;
			}

			AdvanceToNextValue(buffer, bufptr, type, size);
		}
	}

	std::optional<PackReader::WalkToResult> PackReader::WalkToImpl(std::string_view key) {
		auto* bufptr = buffer;

		// Swap elements
		auto nrElements = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
		SafeAdvance(buffer, bufptr, 4, size);

		WalkToResult res;

		for(auto i = 0; i < nrElements; ++i) {
			auto elementNameLength = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			SafeAdvance(buffer, bufptr, 4, size);

			auto elementName = std::string_view(reinterpret_cast<const char*>(bufptr), elementNameLength - 1);
			SafeAdvance(buffer, bufptr, elementNameLength - 1, size);

			auto elementType = static_cast<ValueType>(Swap(*reinterpret_cast<std::uint32_t*>(bufptr)));
			SafeAdvance(buffer, bufptr, 4, size);

			auto elementNumValues = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			SafeAdvance(buffer, bufptr, 4, size);

			// Initalize result with the required fields:
			// - Value Type
			// - Value Count
			// - A pointer to the start of the serialized values
			res.type = elementType;
			res.nrValues = elementNumValues;
			res.valueMemory = bufptr;

			// We found what the caller wanted us to find.
			if(elementName == key) {
				return res;
			}

			// Skip values, we don't care about that
			for(auto j = 0; j < elementNumValues; ++j) {
				AdvanceToNextValue(buffer, bufptr, elementType, size);
			}
		}

		// Key was never found.
		return std::nullopt;
	}

	std::vector<PackReader::ElementKeyT> PackReader::Keys() {
		std::vector<ElementKeyT> ret;
		ElementKeyT push;
		WalkAll([&](ValueType type, std::string_view name, std::size_t size, std::uint8_t* buffer) {
			// Skip values
			if(name == push.key && type == push.elementType) {
				return;
			} else {
				// Setup new element
				push.key = name;
				push.elementType = type;
				ret.push_back(push);
			}
		});

		return ret;
	}

} // namespace vpngate_io::impl
#include <stdexcept>
#include <string_view>
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

	std::optional<std::vector<Value>> PackReader::GetValues(std::string_view key, ValueType expectedType) {
		if(auto res = WalkToImpl(key); res.has_value()) {
			std::vector<Value> ret;
			auto& r = res.value();

			// Wrong type provided.
			if(r.type != expectedType)
				return std::nullopt;

			ret.reserve(r.nrValues);

			struct WalkContext {
				std::vector<Value>& values;
				ValueType type;
			};

			WalkContext ctx {
				ret,
				r.type
			};

			WalkValuesImpl(res->valueMemory, r.type, r.nrValues, [](void* user, std::size_t index, std::size_t size, std::uint8_t* buffer) {
				auto& ctx = *static_cast<WalkContext*>(user);

				Value v {
					.type = ctx.type
				};

				using enum ValueType;
				switch(ctx.type) {
					case Int:
						v.intValue = Swap(*reinterpret_cast<std::uint32_t*>(buffer));
						break;
					case Data:
						v.dataValue = { buffer, size };
						break;
					case String: {
						if(size == 0) {
							v.stringValue = "";
						} else {
							v.stringValue = std::string_view { reinterpret_cast<const char*>(buffer), size };
						}
					} break;
					case WString: {
						if(size == 0) {
							v.wstringValue = "";
						} else {
							v.wstringValue = std::string_view { reinterpret_cast<const char*>(buffer), size };
						}
					} break;
					case Int64:
						v.int64Value = Swap(*reinterpret_cast<std::uint64_t*>(buffer));
						break;
				}

				ctx.values.push_back(v); }, &ctx);

			return ret;
		}

		return std::nullopt;
	}

	void PackReader::WalkValuesImpl(std::uint8_t* pValueStart, ValueType type, std::size_t nrValues, void (*func)(void* user, std::size_t, std::size_t, std::uint8_t*), void* user) {
		auto bufptr = pValueStart;
		auto bufsize = size - (pValueStart - bufptr);

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

			AdvanceToNextValue(buffer, bufptr, type, bufsize);
		}
	}

	std::optional<PackReader::KeyData> PackReader::WalkToImpl(std::string_view key) {
		auto* bufptr = buffer;

		// Swap elements
		auto nrElements = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
		SafeAdvance(buffer, bufptr, 4, size);

		KeyData res;

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
			res.key = elementName;

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

	std::vector<PackReader::KeyData> PackReader::WalkKeysImpl() {
		auto* bufptr = buffer;

		// Swap elements
		auto nrElements = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
		SafeAdvance(buffer, bufptr, 4, size);

		std::vector<KeyData> res;

		res.reserve(nrElements);

		for(auto i = 0; i < nrElements; ++i) {
			auto elementNameLength = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			SafeAdvance(buffer, bufptr, 4, size);

			auto elementName = std::string_view(reinterpret_cast<const char*>(bufptr), elementNameLength - 1);
			SafeAdvance(buffer, bufptr, elementNameLength - 1, size);

			auto elementType = static_cast<ValueType>(Swap(*reinterpret_cast<std::uint32_t*>(bufptr)));
			SafeAdvance(buffer, bufptr, 4, size);

			auto elementNumValues = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
			SafeAdvance(buffer, bufptr, 4, size);

			KeyData data;
			data.type = elementType;
			data.nrValues = elementNumValues;
			data.valueMemory = bufptr;
			data.key = elementName;

			res.push_back(data);

			// Skip values, we don't care about that
			for(auto j = 0; j < elementNumValues; ++j) {
				AdvanceToNextValue(buffer, bufptr, elementType, size);
			}
		}

		return res;
	}

	std::vector<PackReader::ElementKeyT> PackReader::Keys() {
		auto keys = WalkKeysImpl();
		std::vector<ElementKeyT> ret;

		for(auto& key : keys)
			ret.push_back(ElementKeyT {
			.key = key.key,
			.elementType = key.type });

		return ret;
	}

} // namespace vpngate_io::impl
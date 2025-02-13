#pragma once

#include <bit>
#include <cstdio>
#include <optional>
#include <span>
#include <vector>
#include <vpngate_io/value_types.hpp>

namespace vpngate_io {

	namespace impl {

		// FIXME: Move
		template <class T>
		constexpr T Swap(const T value) {
			if constexpr(std::endian::native == std::endian::little)
				return std::byteswap(value);
			return value;
		}

		/// Reader for SoftEther Mayaqua "Pack" files
		///
		/// # Notes
		/// This currently only handles enough required to work with
		/// the VPNGate.dat file. It should also be refactored to maybe
		/// use a byte stream, but for now, this works.
		struct PackReader {
			PackReader(std::uint8_t* buffer, std::size_t size)
				: buffer(buffer), size(size) {
			}

			template <class F>
			void WalkAll(F&& f) {
				auto* bufptr = buffer;

				// Swap elements
				auto nrElements = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
				bufptr += 4;

				for(auto i = 0; i < nrElements; ++i) {
					auto elementNameLength = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
					bufptr += 4;

					auto elementName = std::string_view(reinterpret_cast<const char*>(bufptr), elementNameLength - 1);
					bufptr += elementNameLength - 1;

					auto elementType = static_cast<ValueType>(Swap(*reinterpret_cast<std::uint32_t*>(bufptr)));
					bufptr += 4;

					auto elementNumValues = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
					bufptr += 4;

					// Walk all elements
					for(auto j = 0; j < elementNumValues; ++j) {
						switch(elementType) {
							case ValueType::Int: {
								f(elementType, elementName, 4, bufptr);
								bufptr += 4;
							} break;

							case ValueType::Data: {
								auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
								bufptr += 4;

								f(elementType, elementName, dataSize, bufptr);

								// Walk next value
								bufptr += dataSize;
							} break;

							case ValueType::String: {
								auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
								f(elementType, elementName, dataSize - 1, bufptr + 4);

								// Walk next value
								bufptr += 4 + (dataSize);
							} break;

							case ValueType::WString: {
								auto dataSize = Swap(*reinterpret_cast<std::uint32_t*>(bufptr));
								f(elementType, elementName, dataSize - 1, bufptr + 4);

								// Walk next value
								bufptr += 4 + (dataSize);
							} break;

							case ValueType::Int64: {
								f(elementType, elementName, 8, bufptr);
								bufptr += 8;
							} break;

							default:
								break;
						}
					}
				}
			}

			// TODO: Not this, just name the struct properly
			// I just was debating to use a pair for this or a tuple
			// or something
			using ElementKeyT = struct {
				std::string_view key;
				ValueType elementType;
			};

			std::vector<ElementKeyT> Keys() {
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

			/// Returns `true` if the specified key exists
			template <ValueType Type>
			bool KeyExists(std::string_view key) {
				bool ret = false;

				// Walk the keys; if the key exists AND has the type the user has specified, update ret to true
				WalkAll([&](ValueType type, std::string_view name, std::size_t size, std::uint8_t* buffer) {
					// Skip values
					if(ret)
						return;

					if(name == key && type == Type) {
						ret = true;
					}
				});
				return ret;
			}

			template <ValueType Type>
			auto Get(std::string_view key) -> std::vector<typename ValueTypeToNaturalType<Type>::Type> {
				using T = typename ValueTypeToNaturalType<Type>::Type;
				std::vector<T> ret;

				WalkAll([&](ValueType type, std::string_view name, std::size_t size, std::uint8_t* buffer) {
					if(name == key && type == Type) {
						// Can't really do this in a better way I don't think
						// These will compile out anyways depending on the type so
						// I don't think it's worth bothering

						if constexpr(Type == ValueType::Int) {
							auto swapped = Swap(*reinterpret_cast<std::uint32_t*>(buffer));
							ret.push_back(swapped);
						}

						if constexpr(Type == ValueType::Data) {
							ret.push_back({ buffer, size });
						}

						if constexpr(Type == ValueType::String) {
							if(size == 0) {
								ret.push_back("");
							} else {
								ret.push_back(std::string_view { reinterpret_cast<const char*>(buffer), size });
							}
						}

						if constexpr(Type == ValueType::WString) {
							if(size == 0) {
								ret.push_back("");
							} else {
								ret.push_back(std::string_view { reinterpret_cast<const char*>(buffer), size });
							}
						}
					}
				});

				return ret;
			}

			template <ValueType Type>
			auto GetFirst(std::string_view key) -> std::optional<typename ValueTypeToNaturalType<Type>::Type> {
				auto values = Get<Type>(key);
				if(values.empty())
					return std::nullopt;
				return values[0];
			}

		   private:
			std::uint8_t* buffer;
			std::size_t size;
		};
	} // namespace impl

	using impl::PackReader;

} // namespace vpngate_io

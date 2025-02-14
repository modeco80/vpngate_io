#pragma once

#include <bit>
#include <cstddef>
#include <cstdio>
#include <optional>
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

		/// Reader for SoftEther Mayaqua "Pack" serialized data
		///
		/// # Notes
		/// This should probably be refactored to maybe
		/// use a byte stream, but for now, this works, and
		/// the data also gets to be ✨zero copy✨. Which is nice.
		struct PackReader {
			PackReader(std::uint8_t* buffer, std::size_t size)
				: buffer(buffer), size(size) {
			}

		   public:
			/// Structure for [PackReader::Keys()]
			struct ElementKeyT {
				std::string_view key;
				ValueType elementType;
			};

			/// Gets all keys and their type.
			std::vector<ElementKeyT> Keys();

			/// Returns `true` if the given key exists.
			///
			/// Optionally, type can be set to a value, and this function will also type check, and return false
			/// if the key's type does not match the passed-in type.
			bool KeyExists(std::string_view key, std::optional<ValueType> type = std::nullopt) {
				if(auto res = WalkToImpl(key); res.has_value()) {
					if(type.has_value()) {
						if(res.value().type == type.value()) {
							return true;
						} else {
							return false;
						}
					} else {
						return true;
					}
					
				} else {
					return false;
				}
			}

			std::optional<std::size_t> ValueCount(std::string_view key) {
				if(auto res = WalkToImpl(key); res.has_value()) {
					auto& r = res.value();
					return r.nrValues;
				}
				
				return std::nullopt;
			}

			// FIXME: This used to return a optional. Perhaps it still should

			/// Gets all the values for a key. Returns an empty vector if a key does not exist
			template <ValueType Type>
			auto Get(std::string_view key) -> std::vector<typename ValueTypeToNaturalType<Type>::Type> {
				using T = typename ValueTypeToNaturalType<Type>::Type;
				std::vector<T> ret;

				if(auto res = WalkToImpl(key); res.has_value()) {
					auto& r = res.value();

					// Wrong type provided @ compile time.
					if(r.type != Type)
						return ret;

					ret.resize(r.nrValues);

					WalkValuesImpl(r.valueMemory, Type, r.nrValues, [](void* user, std::size_t index, std::size_t size, std::uint8_t* buffer) {
						auto& ret = *static_cast<std::vector<T>*>(user);

						// Can't really do this in a better way I don't think
						// These will compile out anyways depending on the type so
						// I don't think it's worth bothering

						if constexpr(Type == ValueType::Int) {
							auto swapped = Swap(*reinterpret_cast<std::uint32_t*>(buffer));
							ret[index] = swapped;
						}

						if constexpr(Type == ValueType::Data) {
							ret[index] = { buffer, size };
						}

						if constexpr(Type == ValueType::String) {
							if(size == 0) {
								ret[index] = "";
							} else {
								ret[index] = std::string_view { reinterpret_cast<const char*>(buffer), size };
							}
						}

						if constexpr(Type == ValueType::WString) {
							if(size == 0) {
								ret[index] = "";
							} else {
								ret[index] = std::string_view { reinterpret_cast<const char*>(buffer), size };
							}
						}

						if constexpr(Type == ValueType::Int64) {
							auto swapped = Swap(*reinterpret_cast<std::uint64_t*>(buffer));
							ret[index] = swapped;
						} }, &ret);
				} else {
					// Key was not found
					return ret;
				}

				return ret;
			}

			/// Returns the first value for a key, or nullopt if the key does not exist.
			template <ValueType Type>
			auto GetFirst(std::string_view key) -> std::optional<typename ValueTypeToNaturalType<Type>::Type> {
				auto values = Get<Type>(key);
				if(values.empty())
					return std::nullopt;
				return values[0];
			}

		   private:
			struct KeyData {
				std::string_view key;
				ValueType type;
				std::uint32_t nrValues;
				std::uint8_t* valueMemory;
			};

			std::optional<KeyData> WalkToImpl(std::string_view key);

			std::vector<KeyData> WalkKeysImpl();

			void WalkValuesImpl(std::uint8_t* pValueStart, ValueType type, std::size_t nrValues, void (*func)(void* user, std::size_t, std::size_t, std::uint8_t*), void* user);

			std::uint8_t* buffer;
			std::size_t size;
		};
	} // namespace impl

	using impl::PackReader;

} // namespace vpngate_io

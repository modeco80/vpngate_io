#pragma once

#include <bit>
#include <cstddef>
#include <cstdio>
#include <optional>
#include <vector>
#include <vpngate_io/value_types.hpp>

namespace vpngate_io {

	namespace impl {


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

			/// Gets all the values for a key. Returns a vector of Values (which will need to be handled appropiately)
			/// if the key existed, or nullopt if it did not.
			std::optional<std::vector<Value>> GetValue(std::string_view key, ValueType expectedType);

			std::optional<Value> GetFirstValue(std::string_view key, ValueType expectedType);

			/// Gets all the values for a key. Returns an empty vector if a key does not exist
			template <ValueType Type>
			auto Get(std::string_view key) -> std::vector<typename ValueTypeToNaturalType<Type>::Type> {
				using T = typename ValueTypeToNaturalType<Type>::Type;
				std::vector<T> ret;

				if(auto res = GetValue(key, Type); res.has_value()) {
					auto& r = res.value();

					ret.resize(r.size());

					for(std::size_t i = 0; i < r.size(); ++i) {
						ret[i] = r[i].Cast<Type>();
					}

					return ret;
				}

				return ret;
			}

			/// Returns the first value for a key, or nullopt if the key does not exist.
			template <ValueType Type>
			auto GetFirst(std::string_view key) -> std::optional<typename ValueTypeToNaturalType<Type>::Type> {
				if(auto value = GetFirstValue(key, Type); value.has_value())
					return (*value).Cast<Type>();
				return std::nullopt;
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

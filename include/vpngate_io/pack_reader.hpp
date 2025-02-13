#pragma once

#include <bit>
#include <cstdio>
#include <optional>
#include <span>
#include <string_view>
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

		   public:
			template <class F>
			void WalkAll(F&& f) {
				// We implement this in the .cpp file to keep binary size down.
				WalkAllImpl([](void* user, ValueType type, std::string_view name, std::size_t size, std::uint8_t* bufPtr) {
					(*static_cast<F*>(user))(type, name, size, bufPtr);
				},
							&f);
			}

			// TODO: Not this, just name the struct properly
			// I just was debating to use a pair for this or a tuple
			// or something
			using ElementKeyT = struct {
				std::string_view key;
				ValueType elementType;
			};

			std::vector<ElementKeyT> Keys();

			/// Returns `true` if the specified key exists
			template <ValueType Type>
			bool KeyExists(std::string_view key) {
				bool ret = false;

				// Walk the keys; if the key exists AND has the type the user has specified, update ret to true
				WalkAll([&](ValueType type, std::string_view name, std::size_t size, std::uint8_t* buffer) {
					// Skip once we have walked a single key which matches.
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

						if constexpr(Type == ValueType::Int64) {
							auto swapped = Swap(*reinterpret_cast<std::uint64_t*>(buffer));
							ret.push_back(swapped);
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
			void WalkAllImpl(void (*Func)(void*, ValueType, std::string_view, std::size_t, std::uint8_t*), void* user);

			std::uint8_t* buffer;
			std::size_t size;
		};
	} // namespace impl

	using impl::PackReader;

} // namespace vpngate_io

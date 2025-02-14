#pragma once

#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <system_error>

struct File {
	/// Opens a file. The file always has O_CLOEXEC enabled.
	static File Open(const char* path, int mode) {
		if(auto fd = open(path, mode | O_CLOEXEC); fd != -1) {
			return File(fd);
		} else {
			// errno is mappable to system_category
			throw std::system_error { errno, std::generic_category() };
		}
	}

	// FIXME: use clone() to clone
	File(const File&) = delete;

	File(File&& m) {
		// move ownership of fd to us
		fd = m.fd;
		m.fd = -1;
	}

	~File() {
		Close();
	}

	std::uint64_t Read(void* buffer, std::size_t length) {
		return read(fd, buffer, length);
	}

	std::uint64_t Write(const void* buffer, std::size_t length) {
		return write(fd, buffer, length);
	}

	std::uint64_t Seek(std::uint64_t offset, int whence) {
		return lseek64(fd, offset, whence);
	}

	std::uint64_t Tell() {
		return Seek(0, SEEK_CUR);
	}

	/// Close this file, if it hasn't been closed
	/// or otherwise invalidated already
	void Close() {
		if(fd != -1) {
			close(fd);
			fd = -1;
		}
	}

	std::uint64_t Size() {
		return size;
	}

	std::string ReadLine() {
		std::string str;

		char rn[3] {};
		uint32_t rnindex = 0;

		while(true) {
			char c;

			if(Read(&c, sizeof(c)) != sizeof(c))
				break;

			// probably world's worst scanning machinery for this
			// but it works:tm:
			if(c == '\r' || c == '\n') {
				rn[rnindex++] = c;

				if(rnindex == 2) {
					if(rn[0] == '\r' && rn[1] == '\n') {
						break;
					} else {
						rnindex = 0;
					}
				}
			} else {
				str.push_back(c);
			}
		}

		return str;
	}

   private:
	File(int fd)
		: fd(fd) {
		// Cache size.
		Seek(0, SEEK_END);
		size = Tell();
		Seek(0, SEEK_SET);
	}

	int fd;
	std::uint64_t size;
};
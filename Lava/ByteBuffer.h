#pragma once

#include <cstdint>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct ByteBuffer {
public:
	void readFromFile(const std::filesystem::path& filename);

	std::uint8_t getUI1(std::size_t position) const {
		if (position < this->bytes.size()) return this->bytes[position];
		return 0;
	}
	std::uint16_t getUI2(std::size_t position) const { return getUI1(position) << 8 | getUI1(position + 1); }
	std::uint32_t getUI4(std::size_t position) const { return getUI2(position) << 16 | getUI2(position + 2); }
	std::uint64_t getUI8(std::size_t position) const { return static_cast<std::uint64_t>(getUI4(position)) << 32 | getUI4(position + 4); }
	std::int8_t getI1(std::size_t position) const { return static_cast<std::int8_t>(getUI1(position)); }
	std::int16_t getI2(std::size_t position) const { return static_cast<std::int16_t>(getUI2(position)); }
	std::int32_t getI4(std::size_t position) const { return static_cast<std::int32_t>(getUI4(position)); }
	std::int64_t getI8(std::size_t position) const { return static_cast<std::int64_t>(getUI8(position)); }

	std::size_t getUI1s(std::vector<std::uint8_t>& vec, std::size_t position, std::size_t length) const;
	std::size_t getUI2s(std::vector<std::uint16_t>& vec, std::size_t position, std::size_t length) const;
	std::size_t getUI4s(std::vector<std::uint32_t>& vec, std::size_t position, std::size_t length) const;
	std::size_t getUI8s(std::vector<std::uint64_t>& vec, std::size_t position, std::size_t length) const;
	std::string_view getString(std::size_t position, std::size_t length) const;
	std::string_view getStringNT(std::size_t position) {
		std::size_t length = 0;
		while ((position + length) < this->bytes.size() && this->bytes[position + length] != 0)
			length++;
		return getString(position, length);
	}

	std::uint8_t getUI1() { return getUI1(this->offset++); }
	std::uint16_t getUI2() {
		std::uint16_t out = getUI2(this->offset);
		this->offset += 2;
		return out;
	}
	std::uint32_t getUI4() {
		std::uint32_t out = getUI4(this->offset);
		this->offset += 4;
		return out;
	}
	std::uint64_t getUI8() {
		std::uint64_t out = getUI8(this->offset);
		this->offset += 8;
		return out;
	}
	std::int8_t getI1() { return static_cast<std::int8_t>(getUI1()); }
	std::int16_t getI2() { return static_cast<std::int16_t>(getUI2()); }
	std::int32_t getI4() { return static_cast<std::int32_t>(getUI4()); }
	std::int64_t getI8() { return static_cast<std::int64_t>(getUI8()); }

	std::size_t getUI1s(std::vector<std::uint8_t>& vec, std::size_t length) {
		std::size_t read = getUI1s(vec, this->offset, length);
		this->offset += read;
		return read;
	}
	std::size_t getUI2s(std::vector<std::uint16_t>& vec, std::size_t length) {
		std::size_t read = getUI2s(vec, this->offset, length);
		this->offset += read * 2;
		return read;
	}
	std::size_t getUI4s(std::vector<std::uint32_t>& vec, std::size_t length) {
		std::size_t read = getUI4s(vec, this->offset, length);
		this->offset += read * 4;
		return read;
	}
	std::size_t getUI8s(std::vector<std::uint64_t>& vec, std::size_t length) {
		std::size_t read = getUI8s(vec, this->offset, length);
		this->offset += read * 8;
		return read;
	}
	std::string_view getString(std::size_t length) {
		std::string_view view = getString(this->offset, length);
		this->offset += view.size();
		return view;
	}
	std::string_view getStringNT() {
		std::string_view view = getStringNT(this->offset);
		this->offset += view.size();
		return view;
	}

private:
	std::size_t offset = 0;
	std::vector<std::uint8_t> bytes;
};
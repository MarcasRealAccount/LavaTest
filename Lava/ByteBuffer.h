#pragma once

#include <cstdint>
#include <cstring>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

struct ByteBuffer {
public:
	void readFromFile(const std::filesystem::path& filename);

	auto getOffset() const { return this->offset; }
	void setOffset(std::size_t offset) { this->offset = offset; }
	auto size() const { return this->bytes.size(); }
	auto data() { return this->bytes.data(); }
	auto data() const { return this->bytes.data(); }

	std::uint8_t getUI1(std::size_t position) const {
		if (position < this->bytes.size()) return this->bytes[position];
		return 0;
	}
	std::uint16_t getUI2(std::size_t position) const { return getUI1(position) | getUI1(position + 1) << 8; }
	std::uint32_t getUI4(std::size_t position) const { return getUI2(position) | getUI2(position + 2) << 16; }
	std::uint64_t getUI8(std::size_t position) const { return getUI4(position) | static_cast<std::uint64_t>(getUI4(position + 4)) << 32; }
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

	void addUI1(std::uint8_t value, std::size_t position) { bytes.insert(bytes.begin() + position, value); }
	void addUI2(std::uint16_t value, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + 2);
		this->bytes.insert(this->bytes.begin() + position, { static_cast<std::uint8_t>(value & 0xFF), static_cast<std::uint8_t>((value >> 8) & 0xFF) });
	}
	void addUI4(std::uint32_t value, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + 4);
		this->bytes.insert(this->bytes.begin() + position, { static_cast<std::uint8_t>(value & 0xFF), static_cast<std::uint8_t>((value >> 8) & 0xFF), static_cast<std::uint8_t>((value >> 16) & 0xFF), static_cast<std::uint8_t>((value >> 24) & 0xFF) });
	}
	void addUI8(std::uint64_t value, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + 8);
		this->bytes.insert(this->bytes.begin() + position, { static_cast<std::uint8_t>(value & 0xFF), static_cast<std::uint8_t>((value >> 8) & 0xFF), static_cast<std::uint8_t>((value >> 16) & 0xFF), static_cast<std::uint8_t>((value >> 24) & 0xFF), static_cast<std::uint8_t>((value >> 32) & 0xFF), static_cast<std::uint8_t>((value >> 40) & 0xFF), static_cast<std::uint8_t>((value >> 48) & 0xFF), static_cast<std::uint8_t>((value >> 56) & 0xFF) });
	}
	void addI1(std::int8_t value, std::size_t position) { addUI1(static_cast<std::uint8_t>(value), position); }
	void addI2(std::int16_t value, std::size_t position) { addUI2(static_cast<std::uint16_t>(value), position); }
	void addI4(std::int32_t value, std::size_t position) { addUI4(static_cast<std::uint32_t>(value), position); }
	void addI8(std::int64_t value, std::size_t position) { addUI8(static_cast<std::uint64_t>(value), position); }
	void addUI1(std::uint8_t value) { addUI1(value, this->bytes.size()); }
	void addUI2(std::uint16_t value) { addUI2(value, this->bytes.size()); }
	void addUI4(std::uint32_t value) { addUI4(value, this->bytes.size()); }
	void addUI8(std::uint64_t value) { addUI8(value, this->bytes.size()); }
	void addI1(std::int8_t value) { addI1(value, this->bytes.size()); }
	void addI2(std::int16_t value) { addI2(value, this->bytes.size()); }
	void addI4(std::int32_t value) { addI4(value, this->bytes.size()); }
	void addI8(std::int64_t value) { addI8(value, this->bytes.size()); }

	void addUI1s(const std::vector<std::uint8_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::uint8_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addUI2s(const std::vector<std::uint16_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::uint16_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addUI4s(const std::vector<std::uint32_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::uint32_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addUI8s(const std::vector<std::uint64_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::uint64_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addI1s(const std::vector<std::int8_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::int8_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addI2s(const std::vector<std::int16_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::int16_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addI4s(const std::vector<std::int32_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::int32_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addI8s(const std::vector<std::int64_t>& bytes, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + bytes.size() * sizeof(std::int64_t));
		this->bytes.insert(this->bytes.begin() + position, reinterpret_cast<const std::uint8_t*>(bytes.data()), reinterpret_cast<const std::uint8_t*>(bytes.data() + bytes.size()));
	}
	void addString(std::string_view str, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + str.size() * sizeof(std::string_view::value_type));
		this->bytes.insert(this->bytes.begin() + position, str.data(), str.data() + str.size());
	}
	void addStringNT(std::string_view str, std::size_t position) {
		this->bytes.reserve(this->bytes.capacity() + str.size() * sizeof(std::string_view::value_type));
		this->bytes.insert(this->bytes.begin() + position, str.data(), str.data() + str.size());
		this->bytes.insert(this->bytes.begin() + position + str.size() + 1, 0);
	}

	void addUI1s(const std::vector<uint8_t>& bytes) { addUI1s(bytes, this->bytes.size()); }
	void addString(std::string_view str) { addString(str, this->bytes.size()); }
	void addStringNT(std::string_view str) { addStringNT(str, this->bytes.size()); }

	void setUI1(std::uint8_t value, std::size_t position) {
		if (position < bytes.size()) bytes[position] = value;
	}
	void setUI2(std::uint16_t value, std::size_t position) {
		if (position + 1 < bytes.size()) {
			bytes[position]     = static_cast<std::uint8_t>(value & 0xFF);
			bytes[position + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
		}
	}
	void setUI4(std::uint32_t value, std::size_t position) {
		if (position + 3 < bytes.size()) {
			bytes[position]     = static_cast<std::uint8_t>(value & 0xFF);
			bytes[position + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
			bytes[position + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
			bytes[position + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
		}
	}
	void setUI8(std::uint64_t value, std::size_t position) {
		if (position + 7 < bytes.size()) {
			bytes[position]     = static_cast<std::uint8_t>(value & 0xFF);
			bytes[position + 1] = static_cast<std::uint8_t>((value >> 8) & 0xFF);
			bytes[position + 2] = static_cast<std::uint8_t>((value >> 16) & 0xFF);
			bytes[position + 3] = static_cast<std::uint8_t>((value >> 24) & 0xFF);
			bytes[position + 4] = static_cast<std::uint8_t>((value >> 32) & 0xFF);
			bytes[position + 5] = static_cast<std::uint8_t>((value >> 40) & 0xFF);
			bytes[position + 6] = static_cast<std::uint8_t>((value >> 48) & 0xFF);
			bytes[position + 7] = static_cast<std::uint8_t>((value >> 56) & 0xFF);
		}
	}
	void setUI1(std::uint8_t value) { setUI1(value, bytes.size()); }
	void setUI2(std::uint16_t value) { setUI2(value, bytes.size()); }
	void setUI4(std::uint32_t value) { setUI4(value, bytes.size()); }
	void setUI8(std::uint64_t value) { setUI8(value, bytes.size()); }

	void setUI1s(const std::vector<std::uint8_t>& bytes, std::size_t position) {
		if ((position + bytes.size() * sizeof(std::uint8_t)) < this->bytes.size())
			std::memcpy(this->bytes.data() + position, bytes.data(), bytes.size() * sizeof(std::uint8_t));
	}
	void setUI2s(const std::vector<std::uint16_t>& bytes, std::size_t position) {
		if ((position + bytes.size() * sizeof(std::uint16_t)) < this->bytes.size())
			std::memcpy(this->bytes.data() + position, bytes.data(), bytes.size() * sizeof(std::uint16_t));
	}
	void setUI4s(const std::vector<std::uint32_t>& bytes, std::size_t position) {
		if ((position + bytes.size() * sizeof(std::uint32_t)) < this->bytes.size())
			std::memcpy(this->bytes.data() + position, bytes.data(), bytes.size() * sizeof(std::uint32_t));
	}
	void setUI8s(const std::vector<std::uint64_t>& bytes, std::size_t position) {
		if ((position + bytes.size() * sizeof(std::uint64_t)) < this->bytes.size())
			std::memcpy(this->bytes.data() + position, bytes.data(), bytes.size() * sizeof(std::uint64_t));
	}
	void setString(std::string_view str, std::size_t position) {
		if ((position + str.size() * sizeof(std::string_view::value_type)) < bytes.size())
			std::memcpy(bytes.data() + position, str.data(), str.size() * sizeof(std::string_view::value_type));
	}
	void setStringNT(std::string_view str, std::size_t position) {
		if ((position + str.size() * sizeof(std::string_view::value_type) + 1) < bytes.size()) {
			std::memcpy(bytes.data() + position, str.data(), str.size() * sizeof(std::string_view::value_type));
			bytes[position + str.size() * sizeof(std::string_view::value_type)] = 0;
		}
	}

private:
	std::size_t offset = 0;
	std::vector<std::uint8_t> bytes;
};
#include "ByteBuffer.h"

#include <fstream>

void ByteBuffer::readFromFile(const std::filesystem::path& filename) {
	std::ifstream stream(filename, std::ios::binary | std::ios::ate);
	if (stream) {
		this->offset         = 0;
		std::size_t filesize = stream.tellg();
		this->bytes.resize(filesize);
		stream.seekg(0);
		stream.read(reinterpret_cast<char*>(this->bytes.data()), filesize);
		stream.close();
	}
}

std::size_t ByteBuffer::getUI1s(std::vector<std::uint8_t>& vec, std::size_t position, std::size_t length) const {
	if (position < this->bytes.size()) {
		length = std::min(this->bytes.size() - position, length);
		vec.resize(length);
		for (std::size_t i = 0; i < length; i++)
			vec[i] = getUI1(position + i);
		return length;
	}
	return 0;
}

std::size_t ByteBuffer::getUI2s(std::vector<std::uint16_t>& vec, std::size_t position, std::size_t length) const {
	if (position < this->bytes.size()) {
		length = std::min(this->bytes.size() - position, length * 2) / 2;
		vec.resize(length);
		for (std::size_t i = 0; i < length; i++)
			vec[i] = getUI1(position + (i * 2));
		return length;
	}
	return 0;
}

std::size_t ByteBuffer::getUI4s(std::vector<std::uint32_t>& vec, std::size_t position, std::size_t length) const {
	if (position < this->bytes.size()) {
		length = std::min(this->bytes.size() - position, length * 4) / 4;
		vec.resize(length);
		for (std::size_t i = 0; i < length; i++)
			vec[i] = getUI1(position + (i * 4));
		return length;
	}
	return 0;
}

std::size_t ByteBuffer::getUI8s(std::vector<std::uint64_t>& vec, std::size_t position, std::size_t length) const {
	if (position < this->bytes.size()) {
		length = std::min(this->bytes.size() - position, length * 8) / 8;
		vec.resize(length);
		for (std::size_t i = 0; i < length; i++)
			vec[i] = getUI1(position + (i * 8));
		return length;
	}
	return 0;
}

std::string_view ByteBuffer::getString(std::size_t position, std::size_t length) const {
	if (position < this->bytes.size()) {
		length = std::min(this->bytes.size() - position, length);
		return std::string_view(reinterpret_cast<const char*>(&this->bytes[position]), length);
	}
	return {};
}
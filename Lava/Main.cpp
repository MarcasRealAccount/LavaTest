#include <cstdint>
#include <cstring>

#include <vector>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <iostream>

#if LAVA_SYSTEM_windows
#include <Windows.h>
#elif LAVA_SYSTEM_linux
#include <sys/mman.h>
#endif

template <class T>
struct ExecutableAllocator {
	using value_type = T;

	ExecutableAllocator() = default;
	template <class U> constexpr ExecutableAllocator(const ExecutableAllocator<U>&) noexcept {}

	[[nodiscard]] T* allocate(std::size_t n) {
		if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
			throw std::bad_array_new_length();
#if LAVA_SYSTEM_windows
		LPVOID memRegion = VirtualAlloc(NULL, n * sizeof(T), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!memRegion)
			throw std::bad_alloc();
		return reinterpret_cast<T*>(memRegion);
#elif LAVA_SYSTEM_linux
		void* memRegion = mmap(nullptr, n * sizeof(T), PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (memRegion == (void*)-1)
			throw std::bad_alloc();
		return reinterpret_cast<T*>(memRegion);
#else
		static_assert(false, "ExecutableAllocator only works on Windows!");
#endif
	}

	void deallocate(T* p, std::size_t n) {
#if LAVA_SYSTEM_windows
		VirtualFree(reinterpret_cast<LPVOID>(p), 0, MEM_RELEASE);
#elif LAVA_SYSTEM_linux
		munmap(p, n);
#else
		static_assert(false, "ExecutableAllocator only works on Windows!");
#endif
	}
};

class ByteBuffer {
public:
	void readFromFile(const std::filesystem::path& filename) {
		this->offset = 0;
		this->bytes.clear();
		std::ifstream stream = std::ifstream(filename, std::ios::binary | std::ios::ate);
		if (!stream) return;
		this->bytes.resize(stream.tellg());
		stream.seekg(0);
		stream.read(reinterpret_cast<char*>(this->bytes.data()), this->bytes.size());
		stream.close();
	}

	void setBytes(std::vector<std::uint8_t>&& bytes) {
		this->offset = 0;
		this->bytes = std::move(bytes);
	}

	std::uint8_t getUI1(std::size_t position) {
		if (position < this->bytes.size())
			return this->bytes[position];
		return 0;
	}
	std::uint16_t getUI2(std::size_t position) { return getUI1(position) << 8 | getUI1(position + 1); }
	std::uint32_t getUI4(std::size_t position) { return getUI2(position) << 16 | getUI2(position + 2); }
	std::uint64_t getUI8(std::size_t position) { return static_cast<std::uint64_t>(getUI4(position)) << 32 | getUI4(position + 4); }
	std::int8_t getI1(std::size_t position) { return static_cast<std::int8_t>(getUI1(position)); }
	std::int16_t getI2(std::size_t position) { return static_cast<std::int16_t>(getUI2(position)); }
	std::int32_t getI4(std::size_t position) { return static_cast<std::int32_t>(getUI4(position)); }
	std::int64_t getI8(std::size_t position) { return static_cast<std::int64_t>(getUI8(position)); }
	std::uint8_t getUI1() { return getUI1(this->offset++); }
	std::uint16_t getUI2() { return getUI1() << 8 | getUI1(); }
	std::uint32_t getUI4() { return getUI2() << 16 | getUI2(); }
	std::uint64_t getUI8() { return static_cast<std::uint64_t>(getUI4()) << 32 | getUI4(); }
	std::int8_t getI1() { return static_cast<std::int8_t>(getUI1()); }
	std::int16_t getI2() { return static_cast<std::int16_t>(getUI2()); }
	std::int32_t getI4() { return static_cast<std::int32_t>(getUI4()); }
	std::int64_t getI8() { return static_cast<std::int64_t>(getUI8()); }

	std::size_t getUI1s(std::vector<std::uint8_t>& vec, std::size_t position, std::size_t length) {
		if (position < this->bytes.size()) {
			length = std::min(this->bytes.size() - position, length);
			vec.resize(length);
			for (std::size_t i = 0; i < length; i++) vec[i] = getUI1(position + i);
			return length;
		}
		return 0;
	}
	std::size_t getUI1s(std::vector<std::uint8_t>& vec, std::size_t length) {
		length = getUI1s(vec, this->offset, length);
		this->offset += length;
		return length;
	}
	std::size_t getUI2s(std::vector<std::uint16_t>& vec, std::size_t position, std::size_t length) {
		if (position < this->bytes.size()) {
			length = std::min(this->bytes.size() - position, length * 2) / 2;
			vec.resize(length);
			for (std::size_t i = 0; i < length; i++) vec[i] = getUI2(position + (i * 2));
			return length;
		}
		return 0;
	}
	std::size_t getUI2s(std::vector<std::uint16_t>& vec, std::size_t length) {
		length = getUI2s(vec, this->offset, length);
		this->offset += length * 2;
		return length;
	}
	std::size_t getUI4s(std::vector<std::uint32_t>& vec, std::size_t position, std::size_t length) {
		if (position < this->bytes.size()) {
			length = std::min(this->bytes.size() - position, length * 4) / 4;
			vec.resize(length);
			for (std::size_t i = 0; i < length; i++) vec[i] = getUI2(position + (i * 4));
			return length;
		}
		return 0;
	}
	std::size_t getUI4s(std::vector<std::uint32_t>& vec, std::size_t length) {
		length = getUI4s(vec, this->offset, length);
		this->offset += length * 4;
		return length;
	}
	std::size_t getUI8s(std::vector<std::uint64_t>& vec, std::size_t position, std::size_t length) {
		if (position < this->bytes.size()) {
			length = std::min(this->bytes.size() - position, length * 8) / 8;
			vec.resize(length);
			for (std::size_t i = 0; i < length; i++) vec[i] = getUI2(position + (i * 8));
			return length;
		}
		return 0;
	}
	std::size_t getUI8s(std::vector<std::uint64_t>& vec, std::size_t length) {
		length = getUI8s(vec, this->offset, length);
		this->offset += length * 8;
		return length;
	}

	std::string getString(std::size_t position, std::size_t length) {
		if (position < this->bytes.size()) {
			length = std::min(this->bytes.size() - position, length);
			return std::string(reinterpret_cast<char*>(&bytes[position]), length);
		}
		return {};
	}
	std::string getStringNT(std::size_t position) {
		std::size_t length = 0;
		while ((position + length) < this->bytes.size() && bytes[position + length] != 0)
			length++;
		return getString(position, length);
	}
	std::string getString(std::size_t length) {
		std::string view = getString(this->offset, length);
		this->offset += view.size();
		return view;
	}
	std::string getStringNT() {
		std::string view = getStringNT(this->offset);
		this->offset += view.size();
		return view;
	}

private:
	std::size_t offset = 0;
	std::vector<std::uint8_t> bytes;
};

std::vector<std::filesystem::path> classPaths;

struct ClassConstantPoolEntry {
	ClassConstantPoolEntry() : tag(0) {}
	ClassConstantPoolEntry(ClassConstantPoolEntry&& move) noexcept
		: tag(move.tag) {
		switch (tag) {
		case 1:
			data.entries.Class.nameIndex = move.data.entries.Class.nameIndex;
			break;
		case 2:
			data.entries.UTF8.string = std::move(move.data.entries.UTF8.string);
			break;
		}
	}
	ClassConstantPoolEntry& operator=(ClassConstantPoolEntry&& move) noexcept {
		tag = move.tag;
		std::memset(this->data.info, 0, sizeof(this->data.info));
		switch (tag) {
		case 1:
			data.entries.Class.nameIndex = move.data.entries.Class.nameIndex;
			break;
		case 2:
			data.entries.UTF8.string = std::move(move.data.entries.UTF8.string);
			break;
		}
		return *this;
	}
	~ClassConstantPoolEntry() {
		switch (tag) {
		case 2:
			data.entries.UTF8.string.~basic_string();
			break;
		}
	}

	std::uint8_t tag;
	union Data {
		Data() : info{ 0 } {}
		~Data() {}

		union Entries {
			struct Class { // Tag 1
				std::uint16_t nameIndex; // Constant pool index to a UTF8 entry
			} Class;
			struct UTF8 { // Tag 2
				std::string string;
			} UTF8;
		} entries;
		std::uint8_t info[sizeof(entries)];
	} data;
};

struct ClassAttributeEntry {
	std::uint16_t nameIndex = 0;
	std::vector<std::uint8_t> info;
};

struct ClassFieldEntry {
	std::uint16_t accessFlags = 0;
	std::uint16_t nameIndex = 0;
	std::uint16_t descriptorIndex = 0;
	std::vector<ClassAttributeEntry> attributes;
};

struct ClassMethodEntry {
	std::uint16_t accessFlags = 0;
	std::uint16_t nameIndex = 0;
	std::uint16_t descriptorIndex = 0;
	std::vector<ClassAttributeEntry> attributes;
};

bool validateMagicNumber(std::uint32_t magic) { return magic == 0x484F544C; }

bool validateConstantPool(std::vector<ClassConstantPoolEntry>& constantPool) {
	for (std::size_t i = 0; i < constantPool.size(); i++) {
		ClassConstantPoolEntry& entry = constantPool[i];
		switch (entry.tag) {
		case 1: {
			std::uint16_t nameIndex = entry.data.entries.Class.nameIndex;
			if (nameIndex == 0 || nameIndex > constantPool.size() || constantPool[nameIndex - 1].tag != 2) return false; // Class entry was not pointing to a UTF8 entry
			break;
		}
		case 2: break;         // UTF8s are always valid :)
		default: return false; // Any other entries are invalid
		}
	}
	return true;
}

bool validateAttributes(std::vector<ClassAttributeEntry>& attributes, std::vector<ClassConstantPoolEntry>& constantPool) {
	std::size_t codeAttributes = 0;
	for (std::size_t i = 0; i < attributes.size(); i++) {
		ClassAttributeEntry& entry = attributes[i];
		if (entry.nameIndex == 0 || entry.nameIndex > constantPool.size() || constantPool[entry.nameIndex - 1].tag != 2) return false; // Attribute name index does not point to a UTF8 entry
		auto& attributeName = constantPool[entry.nameIndex - 1].data.entries.UTF8.string;
		if (attributeName == "code") {
			if (codeAttributes > 0) return false; // Can only have one code attribute
			codeAttributes++;
		}
	}
	return true;
}

bool validateFields(std::vector<ClassFieldEntry>& fields, std::vector<ClassConstantPoolEntry>& constantPool) {
	for (std::size_t i = 0; i < fields.size(); i++) {
		ClassFieldEntry& entry = fields[i];
		if (entry.nameIndex == 0 || entry.nameIndex > constantPool.size() || constantPool[entry.nameIndex - 1].tag != 2) return false; // Field name index does not point to a UTF8 entry
		if (entry.descriptorIndex == 0 || entry.descriptorIndex > constantPool.size() || constantPool[entry.descriptorIndex - 1].tag != 2) return false; // Field descriptor index does not point to a UTF8 entry
		if (!validateAttributes(entry.attributes, constantPool)) return false;
	}
	return true;
}

bool validateMethods(std::vector<ClassMethodEntry>& methods, std::vector<ClassConstantPoolEntry>& constantPool) {
	for (std::size_t i = 0; i < methods.size(); i++) {
		ClassMethodEntry& entry = methods[i];
		if (entry.nameIndex == 0 || entry.nameIndex > constantPool.size() || constantPool[entry.nameIndex - 1].tag != 2) return false; // Method name index does not point to a UTF8 entry
		if (entry.descriptorIndex == 0 || entry.descriptorIndex > constantPool.size() || constantPool[entry.descriptorIndex - 1].tag != 2) return false; // Method descriptor index does not point to a UTF8 entry
		if (!validateAttributes(entry.attributes, constantPool)) return false;
	}
	return true;
}

class EAccessFlags {
public:
	constexpr EAccessFlags(std::uint16_t value) : value(value) {}

	operator std::uint16_t() const { return this->value; }

	friend EAccessFlags operator+(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value + rhs.value; }
	friend EAccessFlags operator-(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value - rhs.value; }
	friend EAccessFlags operator*(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value * rhs.value; }
	friend EAccessFlags operator/(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value / rhs.value; }
	friend EAccessFlags operator%(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value % rhs.value; }
	friend EAccessFlags operator++(EAccessFlags flags) { return flags.value++; }
	friend EAccessFlags operator--(EAccessFlags flags) { return flags.value--; }
	friend bool operator==(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value == rhs.value; }
	friend bool operator!=(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value != rhs.value; }
	friend bool operator>(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value > rhs.value; }
	friend bool operator<(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value < rhs.value; }
	friend bool operator>=(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value >= rhs.value; }
	friend bool operator<=(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value <= rhs.value; }
	friend EAccessFlags operator&(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value & rhs.value; }
	friend EAccessFlags operator|(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value | rhs.value; }
	friend EAccessFlags operator^(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value ^ rhs.value; }
	friend EAccessFlags operator~(EAccessFlags flags) { return ~flags.value; }
	friend EAccessFlags operator<<(EAccessFlags flags, std::size_t count) { return flags.value << count; }
	friend EAccessFlags operator>>(EAccessFlags flags, std::size_t count) { return flags.value >> count; }
	friend std::ostream& operator<<(std::ostream& stream, EAccessFlags flags);

private:
	std::uint16_t value;
};

namespace EAccessFlag {
	static constexpr EAccessFlags Public = 0x001;
	static constexpr EAccessFlags Private = 0x0002;
	static constexpr EAccessFlags Protected = 0x0004;
	static constexpr EAccessFlags Static = 0x0008;
	static constexpr EAccessFlags Final = 0x0010;
	static constexpr EAccessFlags Super = 0x0020;
	static constexpr EAccessFlags Synchronized = 0x0020;
	static constexpr EAccessFlags Volatile = 0x0040;
	static constexpr EAccessFlags Bridge = 0x0040;
	static constexpr EAccessFlags Transient = 0x0080;
	static constexpr EAccessFlags Varargs = 0x0080;
	static constexpr EAccessFlags Native = 0x0100;
	static constexpr EAccessFlags Interface = 0x0200;
	static constexpr EAccessFlags Abstract = 0x0400;
	static constexpr EAccessFlags Strict = 0x0800;
	static constexpr EAccessFlags Synthetic = 0x1000;
	static constexpr EAccessFlags Annotation = 0x2000;
	static constexpr EAccessFlags Enum = 0x4000;
	static constexpr EAccessFlags Module = 0x8000;
}

std::ostream& operator<<(std::ostream& stream, EAccessFlags flags) {
	bool printed = false;
	if (flags & EAccessFlag::Public) {
		stream << "Public";
		printed = true;
	}
	if (flags & EAccessFlag::Private) {
		if (printed) stream << " | ";
		stream << "Private";
		printed = true;
	}
	if (flags & EAccessFlag::Protected) {
		if (printed) stream << " | ";
		stream << "Protected";
		printed = true;
	}
	if (flags & EAccessFlag::Static) {
		if (printed) stream << " | ";
		stream << "Static";
		printed = true;
	}
	if (flags & EAccessFlag::Final) {
		if (printed) stream << " | ";
		stream << "Final";
		printed = true;
	}
	if (flags & EAccessFlag::Super) {
		if (printed) stream << " | ";
		stream << "Super(Synchronized)";
		printed = true;
	}
	if (flags & EAccessFlag::Volatile) {
		if (printed) stream << " | ";
		stream << "Volatile(Bridge)";
		printed = true;
	}
	if (flags & EAccessFlag::Transient) {
		if (printed) stream << " | ";
		stream << "Transient(Varargs)";
		printed = true;
	}
	if (flags & EAccessFlag::Native) {
		if (printed) stream << " | ";
		stream << "Native";
		printed = true;
	}
	if (flags & EAccessFlag::Interface) {
		if (printed) stream << " | ";
		stream << "Interface";
		printed = true;
	}
	if (flags & EAccessFlag::Abstract) {
		if (printed) stream << " | ";
		stream << "Abstract";
		printed = true;
	}
	if (flags & EAccessFlag::Strict) {
		if (printed) stream << " | ";
		stream << "Strict";
		printed = true;
	}
	if (flags & EAccessFlag::Synthetic) {
		if (printed) stream << " | ";
		stream << "Synthetic";
		printed = true;
	}
	if (flags & EAccessFlag::Annotation) {
		if (printed) stream << " | ";
		stream << "Annotation";
		printed = true;
	}
	if (flags & EAccessFlag::Enum) {
		if (printed) stream << " | ";
		stream << "Enum";
		printed = true;
	}
	if (flags & EAccessFlag::Module) {
		if (printed) stream << " | ";
		stream << "Module";
		printed = true;
	}
	return stream;
}

struct Field {
	std::string name;
	std::string descriptor;
	EAccessFlags accessFlags = EAccessFlag::Public;
};

struct Method {
	std::string name;
	std::string descriptor;
	EAccessFlags accessFlags = EAccessFlag::Public;
	std::vector<std::uint8_t, ExecutableAllocator<std::uint8_t>> code;
};

struct Class {
	std::string name;
	EAccessFlags accessFlags = EAccessFlag::Public;
	std::vector<Class*> supers;
	std::vector<Field> fields;
	std::vector<Method> methods;
};

Class* loadLavaClass(std::string_view lavaClassName);

Class* loadClassVersion1(ByteBuffer& buffer) {
	auto constantPoolCount = buffer.getUI2();
	std::vector<ClassConstantPoolEntry> constantPool(constantPoolCount - 1);
	for (std::size_t i = 0; i < constantPool.size(); i++) {
		ClassConstantPoolEntry& entry = constantPool[i];
		entry.tag = buffer.getUI1();
		switch (entry.tag) {
		case 1: {
			entry.data.entries.Class.nameIndex = buffer.getUI2();
			break;
		}
		case 2: {
			auto length = buffer.getUI4();
			std::string str = buffer.getString(static_cast<std::size_t>(length));
			entry.data.entries.UTF8.string = str;
			break;
		}
		default: return nullptr; // Constant pool entry tag invalid
		}
	}
	if (!validateConstantPool(constantPool)) return nullptr; // One or more Constant Pool entries are invalid
	auto accessFlags = buffer.getUI2();
	auto thisClass = buffer.getUI2();
	if (thisClass == 0 || thisClass > constantPool.size() || constantPool[thisClass - 1].tag != 1) return nullptr; // This class is not pointing to a Class entry
	auto supersCount = buffer.getUI2();
	std::vector<std::uint16_t> supers;
	buffer.getUI2s(supers, supersCount);
	for (std::size_t i = 0; i < supers.size(); i++) {
		std::uint16_t super = supers[i];
		if (super == 0 || super > constantPool.size() || constantPool[super - 1].tag != 1) return nullptr; // Super is not pointing to a Class entry
	}
	auto fieldsCount = buffer.getUI2();
	std::vector<ClassFieldEntry> fields(fieldsCount);
	for (std::size_t i = 0; i < fields.size(); i++) {
		ClassFieldEntry& entry = fields[i];
		entry.accessFlags = buffer.getUI2();
		entry.nameIndex = buffer.getUI2();
		entry.descriptorIndex = buffer.getUI2();
		entry.attributes.resize(buffer.getUI2());
		for (std::size_t j = 0; j < entry.attributes.size(); j++) {
			ClassAttributeEntry& attributeEntry = entry.attributes[j];
			attributeEntry.nameIndex = buffer.getUI2();
			std::uint32_t attributeLength = buffer.getUI4();
			buffer.getUI1s(attributeEntry.info, attributeLength);
		}
	}
	if (!validateFields(fields, constantPool)) return nullptr; // One or more Field entries are invalid
	auto methodsCount = buffer.getUI2();
	std::vector<ClassMethodEntry> methods(methodsCount);
	for (std::size_t i = 0; i < methods.size(); i++) {
		ClassMethodEntry& entry = methods[i];
		entry.accessFlags = buffer.getUI2();
		entry.nameIndex = buffer.getUI2();
		entry.descriptorIndex = buffer.getUI2();
		entry.attributes.resize(buffer.getUI2());
		for (std::size_t j = 0; j < entry.attributes.size(); j++) {
			ClassAttributeEntry& attributeEntry = entry.attributes[j];
			attributeEntry.nameIndex = buffer.getUI2();
			std::uint32_t attributeLength = buffer.getUI4();
			buffer.getUI1s(attributeEntry.info, attributeLength);
		}
	}
	if (!validateMethods(methods, constantPool)) return nullptr; // One or more Method entries are invalid
	auto attributesCount = buffer.getUI2();
	std::vector<ClassAttributeEntry> attributes(attributesCount);
	for (std::size_t i = 0; i < attributes.size(); i++) {
		ClassAttributeEntry& entry = attributes[i];
		entry.nameIndex = buffer.getUI2();
		std::uint32_t attributeLength = buffer.getUI4();
		buffer.getUI1s(entry.info, attributeLength);
	}
	if (!validateAttributes(attributes, constantPool)) return nullptr; // One or more Attribute entries are invalid

	auto clazz = new Class();
	clazz->name = constantPool[constantPool[thisClass - 1].data.entries.Class.nameIndex - 1].data.entries.UTF8.string;
	clazz->accessFlags = accessFlags;
	clazz->supers.resize(supers.size());
	for (std::size_t i = 0; i < supers.size(); i++) {
		auto super = loadLavaClass(constantPool[constantPool[supers[i] - 1].data.entries.Class.nameIndex - 1].data.entries.UTF8.string);
		if (!super) return nullptr; // Super could not be loaded
		clazz->supers[i] = super;
	}
	clazz->fields.resize(fields.size());
	for (std::size_t i = 0; i < fields.size(); i++) {
		auto& entry = fields[i];
		auto& field = clazz->fields[i];
		field.name = constantPool[entry.nameIndex - 1].data.entries.UTF8.string;
		field.descriptor = constantPool[entry.descriptorIndex - 1].data.entries.UTF8.string;
		field.accessFlags = entry.accessFlags;
		for (auto& attribute : entry.attributes) {
			auto& attributeName = constantPool[attribute.nameIndex - 1].data.entries.UTF8.string;
			// TODO: Apply attributes to field
		}
	}
	clazz->methods.resize(methods.size());
	for (std::size_t i = 0; i < methods.size(); i++) {
		auto& entry = methods[i];
		auto& method = clazz->methods[i];
		method.name = constantPool[entry.nameIndex - 1].data.entries.UTF8.string;
		method.descriptor = constantPool[entry.descriptorIndex - 1].data.entries.UTF8.string;
		method.accessFlags = entry.accessFlags;
		for (auto& attribute : entry.attributes) {
			auto& attributeName = constantPool[attribute.nameIndex - 1].data.entries.UTF8.string;
			if (attributeName == "code") {
				method.code.resize(attribute.info.size());
				std::memcpy(method.code.data(), attribute.info.data(), method.code.size());
			}
		}
	}
	return clazz;
}

std::filesystem::path findLavaClass(std::string_view lavaClassName) {
	std::filesystem::path filename;
	for (auto& classPath : classPaths) {
		filename = (classPath / lavaClassName);
		filename.replace_extension("lclass");
		if (std::filesystem::exists(filename))
			return filename;
	}
	return {};
}

Class* loadLavaClass(std::string_view lavaClassName) {
	std::filesystem::path filename = findLavaClass(lavaClassName);
	if (filename.empty()) return nullptr; // LClass file not found

	ByteBuffer bytes;
	bytes.readFromFile(filename);

	std::uint32_t magic = bytes.getUI4();
	if (!validateMagicNumber(magic)) return nullptr; // LCLass file is not an LClass, me sad :,(

	std::uint16_t majorVersion = bytes.getUI2();
	switch (majorVersion) {
	case 1:
		return loadClassVersion1(bytes);
	default: return nullptr; // LClass file is either too new or too old
	}
}

void debugPrintClass(Class* clazz) {
	std::cout << "Class '" << clazz->name << "'\n";
	std::cout << "\tAccess Flags: '" << clazz->accessFlags << "'\n";
	std::cout << "\tSupers:\n";
	for (std::size_t i = 0; i < clazz->supers.size(); i++)
		std::cout << "\t\t'" << clazz->supers[i]->name << "'\n";
	for (std::size_t i = 0; i < clazz->fields.size(); i++) {
		auto& field = clazz->fields[i];
		std::cout << "\tField '" << field.name << "'\n";
		std::cout << "\t\tDescriptor: '" << field.descriptor << "'\n";
		std::cout << "\t\tAccessFlags: '" << field.accessFlags << "'\n";
	}
	for (std::size_t i = 0; i < clazz->methods.size(); i++) {
		auto& method = clazz->methods[i];
		std::cout << "\tMethod '" << method.name << "'\n";
		std::cout << "\t\tDescriptor: '" << method.descriptor << "'\n";
		std::cout << "\t\tAccessFlags: '" << method.accessFlags << "'\n";
		std::cout << "\t\tCode:" << std::hex << std::uppercase << "\n\t\t\t";
		std::size_t column = 0;
		for (std::size_t j = 0; j < method.code.size(); j++) {
			if (column > 0) {
				if ((column % 8) == 0)
					std::cout << "  ";
				else
					std::cout << " ";
			}
			std::cout << static_cast<uint64_t>(method.code[j]);
			column++;
			if (column >= 16) {
				std::cout << "\n\t\t\t";
				column = 0;
			}
		}
		std::cout << "\n" << std::dec << std::nouppercase;
	}
}

template <class R, class... Params>
union FunctionCast {
	using FuncType = R(*)(Params...);

	FunctionCast(FuncType func) : func(func) {}
	FunctionCast(void* ptr) : ptr(ptr) {}

	FuncType func;
	void* ptr;
};

int main() {
	classPaths.push_back(".");
	Class* clazz = loadLavaClass("Test");
	if (!clazz) {
		std::cerr << "Clazz could not be loaded!" << std::endl;
		return EXIT_FAILURE;
	}
	debugPrintClass(clazz);
	int result = FunctionCast<int>(clazz->methods[0].code.data()).func();
	std::cout << "Returned: " << std::hex << std::uppercase << result << std::dec << std::nouppercase << "\n";
}
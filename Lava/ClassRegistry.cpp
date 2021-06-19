#include "ClassRegistry.h"
#include "ByteBuffer.h"

#include <cstring>

#if LAVA_SYSTEM_windows
	#include <Windows.h>
#elif LAVA_SYSTEM_linux
	#include <sys/mman.h>
#else
	#error Requires executable memory allocation, which isnt supported by your system
#endif

static void* allocateReadWriteMemory(std::size_t bytes);
static void makeExecutableMemory(void* p, std::size_t bytes);
static void makeNonExecutableMemory(void* p, std::size_t bytes);
static void deallocateMemory(void* p, std::size_t bytes);

static Class* loadClassV1(ClassRegistry* registry, ByteBuffer& buffer, EClassLoadStatus* loadStatus);

std::ostream& operator<<(std::ostream& stream, EClassLoadStatus status) {
	switch (status) {
	case EClassLoadStatus::Success: return stream << "Success";
	case EClassLoadStatus::FileNotFound: return stream << "FileNotFound";
	case EClassLoadStatus::InvalidMagicNumber: return stream << "InvalidMagicNumber";
	case EClassLoadStatus::InvalidVersion: return stream << "InvalidVersion";
	case EClassLoadStatus::InvalidConstantPool: return stream << "InvalidConstantPool";
	case EClassLoadStatus::InvalidConstantPoolEntry: return stream << "InvalidConstantPoolEntry";
	case EClassLoadStatus::InvalidThisClassEntry: return stream << "InvalidThisClassEntry";
	case EClassLoadStatus::InvalidFieldName: return stream << "InvalidFieldName";
	case EClassLoadStatus::InvalidFieldDescriptor: return stream << "InvalidFieldDescriptor";
	case EClassLoadStatus::InvalidAttributeName: return stream << "InvalidAttributeName";
	case EClassLoadStatus::InvalidMethodName: return stream << "InvalidMethodName";
	case EClassLoadStatus::InvalidMethodDescriptor: return stream << "InvalidMethodDescriptor";
	}
	return stream;
}

void ClassRegistry::addClassPath(const std::filesystem::path classPath) {
	this->classPaths.push_back(classPath);
}

Class* ClassRegistry::loadClass(const std::string& className, EClassLoadStatus* loadStatus) {
	std::filesystem::path filename = findClass(className);
	if (filename.empty()) {
		// Return FileNotFound status.
		if (loadStatus) *loadStatus = EClassLoadStatus::FileNotFound;
		return nullptr;
	}

	ByteBuffer buffer;
	buffer.readFromFile(filename);

	std::uint32_t magic = buffer.getUI4();
	if (magic != 0x484F544C) {
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMagicNumber;
		return nullptr;
	}

	std::uint16_t version = buffer.getUI2();
	switch (version) {
	case 1: return loadClassV1(this, buffer, loadStatus);
	default:
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidVersion;
		return nullptr;
	}
}

std::vector<Class*> ClassRegistry::getLoadedClasses() const {
	std::vector<Class*> classes;
	classes.reserve(this->classes.size());
	for (auto& clazz : this->classes)
		classes.push_back(clazz.second);
	return classes;
}

std::filesystem::path ClassRegistry::findClass(std::string_view className) const {
	for (auto& classPath : this->classPaths) {
		std::filesystem::path filename = classPath / className;
		filename.replace_extension("lclass");
		if (std::filesystem::exists(filename))
			return filename;
	}
	return {};
}

//-------------------
// LClass Version 1
//-------------------

static constexpr std::uint8_t ClassConstantClassEntryV1Tag = 1;
static constexpr std::uint8_t ClassConstantUTF8EntryV1Tag  = 2;

struct ClassConstantPoolEntryV1 {
public:
	ClassConstantPoolEntryV1(std::uint8_t tag) : tag(tag) { }

	std::uint8_t getTag() const { return this->tag; }

private:
	std::uint8_t tag;
};

struct ClassConstantClassEntryV1 : public ClassConstantPoolEntryV1 {
	ClassConstantClassEntryV1(std::uint16_t nameIndex) : ClassConstantPoolEntryV1(ClassConstantClassEntryV1Tag), nameIndex(nameIndex) { }

	std::uint16_t nameIndex;
};

struct ClassConstantUTF8EntryV1 : public ClassConstantPoolEntryV1 {
	ClassConstantUTF8EntryV1(std::string_view string) : ClassConstantPoolEntryV1(ClassConstantUTF8EntryV1Tag), string(string) { }

	std::string string;
};

struct ClassConstantPoolV1 {
public:
	void reserve(std::size_t size) { this->entries.reserve(size); }
	std::size_t size() const { return this->entries.size(); }
	void addEntry(ClassConstantPoolEntryV1* entry) { this->entries.push_back(entry); }
	ClassConstantPoolEntryV1* getEntry(std::size_t index) const {
		if (index == 0 || index > this->entries.size()) return nullptr;
		return this->entries[index - 1];
	}

	bool validate() const {
		for (auto entry : this->entries) {
			switch (entry->getTag()) {
			case ClassConstantClassEntryV1Tag: {
				auto clazzEntry = reinterpret_cast<ClassConstantClassEntryV1*>(entry);
				auto nameEntry  = getEntry(clazzEntry->nameIndex);
				if (!nameEntry || nameEntry->getTag() != ClassConstantUTF8EntryV1Tag)
					return false;
				break;
			}
			case ClassConstantUTF8EntryV1Tag: break;
			default: return false;
			}
		}
		return true;
	}

	auto begin() { return this->entries.begin(); }
	auto begin() const { return this->entries.begin(); }
	auto cbegin() const { return this->entries.cbegin(); }
	auto rbegin() { return this->entries.rbegin(); }
	auto rbegin() const { return this->entries.rbegin(); }
	auto crbegin() const { return this->entries.crbegin(); }
	auto end() { return this->entries.end(); }
	auto end() const { return this->entries.end(); }
	auto cend() const { return this->entries.cend(); }
	auto rend() { return this->entries.rend(); }
	auto rend() const { return this->entries.rend(); }
	auto crend() const { return this->entries.crend(); }

private:
	std::vector<ClassConstantPoolEntryV1*> entries;
};

ClassConstantPoolEntryV1* readConstantPoolEntryV1(ByteBuffer& buffer, EClassLoadStatus* loadStatus) {
	std::uint8_t tag = buffer.getUI1();
	switch (tag) {
	case ClassConstantClassEntryV1Tag: return new ClassConstantClassEntryV1(buffer.getUI2());
	case ClassConstantUTF8EntryV1Tag: {
		std::uint32_t length = buffer.getUI4();
		return new ClassConstantUTF8EntryV1(buffer.getString(length));
	}
	default:
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidConstantPoolEntry;
		return nullptr;
	}
}

struct ClassAttributeEntryV1 {
	std::string name;
	std::vector<std::uint8_t> info;
};

struct ClassFieldEntryV1 {
	EAccessFlags accessFlags = 0;
	std::string name;
	std::string descriptor;
	std::vector<ClassAttributeEntryV1> attributes;
};

struct ClassMethodEntryV1 {
	EAccessFlags accessFlags = 0;
	std::string name;
	std::string descriptor;
	std::vector<ClassAttributeEntryV1> attributes;
};

ClassAttributeEntryV1 readAttributeEntryV1(ByteBuffer& buffer, ClassConstantPoolV1& constantPool, EClassLoadStatus* loadStatus) {
	ClassAttributeEntryV1 attribute;
	auto attributeNameEntry = constantPool.getEntry(buffer.getUI2());
	if (!attributeNameEntry || attributeNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidAttributeName;
		return {};
	}
	attribute.name = reinterpret_cast<ClassConstantUTF8EntryV1*>(attributeNameEntry)->string;

	std::uint32_t attributeLength = buffer.getUI4();
	buffer.getUI1s(attribute.info, attributeLength);
	return attribute;
}

Class* loadClassV1(ClassRegistry* registry, ByteBuffer& buffer, EClassLoadStatus* loadStatus) {
	ClassConstantPoolV1 constantPool;
	std::uint16_t constantPoolSize = buffer.getUI2();
	constantPool.reserve(constantPoolSize - 1);
	for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(constantPoolSize) - 1; i++) {
		auto entry = readConstantPoolEntryV1(buffer, loadStatus);
		if (!entry) return nullptr;
		constantPool.addEntry(entry);
	}

	if (!constantPool.validate()) {
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidConstantPool;
		return nullptr;
	}

	EAccessFlags accessFlags = buffer.getUI2();
	auto thisClassEntry      = constantPool.getEntry(buffer.getUI2());
	if (!thisClassEntry || thisClassEntry->getTag() != ClassConstantClassEntryV1Tag) {
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidThisClassEntry;
		return nullptr;
	}

	std::uint16_t superCount = buffer.getUI2();
	std::vector<std::uint16_t> supers;
	buffer.getUI2s(supers, superCount);
	for (auto super : supers) {
		auto superEntry = constantPool.getEntry(super);
		if (!superEntry || superEntry->getTag() != ClassConstantClassEntryV1Tag) {
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidThisClassEntry;
			return nullptr;
		}
	}

	std::uint16_t fieldCount = buffer.getUI2();
	std::vector<ClassFieldEntryV1> fields(fieldCount);
	for (std::size_t i = 0; i < fields.size(); i++) {
		auto& field       = fields[i];
		field.accessFlags = buffer.getUI2();

		auto fieldNameEntry = constantPool.getEntry(buffer.getUI2());
		if (!fieldNameEntry || fieldNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidFieldName;
			return nullptr;
		}
		field.name = reinterpret_cast<ClassConstantUTF8EntryV1*>(fieldNameEntry)->string;

		auto fieldDescriptorEntry = constantPool.getEntry(buffer.getUI2());
		if (!fieldDescriptorEntry || fieldDescriptorEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidFieldDescriptor;
			return nullptr;
		}
		field.descriptor = reinterpret_cast<ClassConstantUTF8EntryV1*>(fieldDescriptorEntry)->string;

		std::uint16_t attributeCount = buffer.getUI2();
		field.attributes.resize(attributeCount);
		for (std::size_t j = 0; j < field.attributes.size(); j++) {
			EClassLoadStatus loadStatus2 = EClassLoadStatus::Success;
			field.attributes[j]          = readAttributeEntryV1(buffer, constantPool, &loadStatus2);
			if (loadStatus2 != EClassLoadStatus::Success) {
				if (loadStatus) *loadStatus = loadStatus2;
				return nullptr;
			}
		}
	}

	std::uint16_t methodCount = buffer.getUI2();
	std::vector<ClassMethodEntryV1> methods(methodCount);
	for (std::size_t i = 0; i < methods.size(); i++) {
		auto& method       = methods[i];
		method.accessFlags = buffer.getUI2();

		auto methodNameEntry = constantPool.getEntry(buffer.getUI2());
		if (!methodNameEntry || methodNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMethodName;
			return nullptr;
		}
		method.name = reinterpret_cast<ClassConstantUTF8EntryV1*>(methodNameEntry)->string;

		auto methodDescriptorEntry = constantPool.getEntry(buffer.getUI2());
		if (!methodDescriptorEntry || methodDescriptorEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMethodDescriptor;
			return nullptr;
		}
		method.descriptor = reinterpret_cast<ClassConstantUTF8EntryV1*>(methodDescriptorEntry)->string;

		std::uint16_t attributeCount = buffer.getUI2();
		method.attributes.resize(attributeCount);
		for (std::size_t j = 0; j < method.attributes.size(); j++) {
			EClassLoadStatus loadStatus2 = EClassLoadStatus::Success;
			method.attributes[j]         = readAttributeEntryV1(buffer, constantPool, &loadStatus2);
			if (loadStatus2 != EClassLoadStatus::Success) {
				if (loadStatus) *loadStatus = loadStatus2;
				return nullptr;
			}
		}
	}

	std::uint16_t attributeCount = buffer.getUI2();
	std::vector<ClassAttributeEntryV1> attributes(attributeCount);
	for (std::size_t j = 0; j < attributes.size(); j++) {
		EClassLoadStatus loadStatus2 = EClassLoadStatus::Success;
		attributes[j]                = readAttributeEntryV1(buffer, constantPool, &loadStatus2);
		if (loadStatus2 != EClassLoadStatus::Success) {
			if (loadStatus) *loadStatus = loadStatus2;
			return nullptr;
		}
	}

	Class* clazz       = new Class();
	clazz->accessFlags = accessFlags;

	auto thisClass     = reinterpret_cast<ClassConstantClassEntryV1*>(thisClassEntry);
	auto thisClassName = reinterpret_cast<ClassConstantUTF8EntryV1*>(constantPool.getEntry(thisClass->nameIndex));
	clazz->name        = thisClassName->string;

	clazz->supers.resize(supers.size());
	for (std::size_t i = 0; i < supers.size(); i++) {
		auto super      = reinterpret_cast<ClassConstantClassEntryV1*>(constantPool.getEntry(supers[i]));
		auto superName  = reinterpret_cast<ClassConstantUTF8EntryV1*>(constantPool.getEntry(super->nameIndex));
		auto superClass = registry->loadClass(superName->string, loadStatus);
		if (!superClass) return nullptr;

		clazz->supers[i] = superClass;
	}

	clazz->fields.resize(fields.size());
	for (std::size_t i = 0; i < fields.size(); i++) {
		auto& field       = clazz->fields[i];
		auto& entry       = fields[i];
		field.accessFlags = entry.accessFlags;
		field.name        = entry.name;
		field.descriptor  = entry.descriptor;
		// TODO: Apply field attributes
	}

	clazz->methods.resize(methods.size());
	for (std::size_t i = 0; i < methods.size(); i++) {
		auto& method       = clazz->methods[i];
		auto& entry        = methods[i];
		method.accessFlags = entry.accessFlags;
		method.name        = entry.name;
		method.descriptor  = entry.descriptor;
		for (auto& attribute : entry.attributes) {
			if (attribute.name == "code") {
				method.codeLength = attribute.info.size();
				method.pCode      = reinterpret_cast<std::uint8_t*>(allocateReadWriteMemory(method.codeLength));
				std::memcpy(method.pCode, attribute.info.data(), method.codeLength);
				makeExecutableMemory(method.pCode, method.codeLength);
			}
		}
	}

	if (loadStatus) *loadStatus = EClassLoadStatus::Success;
	return clazz;
}

//----------------------------
// Windows execute allocation
//----------------------------

#if LAVA_SYSTEM_windows
void* allocateReadWriteMemory(std::size_t bytes) {
	return VirtualAlloc(nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void makeExecutableMemory(void* p, std::size_t bytes) {
	DWORD old;
	VirtualProtect(p, bytes, PAGE_EXECUTE_READ, &old);
}

void makeNonExecutableMemory(void* p, std::size_t bytes) {
	DWORD old;
	VirtualProtect(p, bytes, PAGE_READWRITE, &old);
}

void deallocateMemory(void* p, std::size_t bytes) {
	VirtualFree(p, 0, MEM_RELEASE);
}
#endif

//----------------------------
// Linux execute allocation
//----------------------------

#if LAVA_SYSTEM_linux
void* allocateReadWriteMemory(std::size_t bytes) {
	return mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void makeExecutableMemory(void* p, std::size_t bytes) {
	mprotect(p, bytes, PROT_EXEC | PROT_READ);
}

void makeNonExecutableMemory(void* p, std::size_t bytes) {
	mprotect(p, bytes, PROT_READ | PROT_WRITE);
}

void deallocateMemory(void* p, std::size_t bytes) {
	munmap(p, bytes);
}
#endif
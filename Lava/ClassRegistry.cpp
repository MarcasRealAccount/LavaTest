#include "ClassRegistry.h"
#include "ByteBuffer.h"

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
		// .lclass file was not found
		if (loadStatus) *loadStatus = EClassLoadStatus::FileNotFound;
		return nullptr;
	}

	// Read .lclass file into a ByteBuffer
	ByteBuffer buffer;
	buffer.readFromFile(filename);

	// Read magic number and check that it is the string "HOTL"
	std::uint32_t magic = buffer.getUI4();
	if (magic != 0x484F544C) {
		// Magic number is not the string "HOTL"
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMagicNumber;
		return nullptr;
	}

	// Read version and load class using that version
	std::uint16_t version = buffer.getUI2();
	switch (version) {
	case 1: return loadClassV1(this, buffer, loadStatus);
	default:
		// Version is not one of the loadable versions
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
	// Loop through every path in the class paths
	// And concatenate the class name and .lclass to it
	// Check if the file exists and return it if so
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
		// Loop through all entries in the constant pool
		// Check if it is a valid tag and if the entry is valid
		for (auto entry : this->entries) {
			switch (entry->getTag()) {
			case ClassConstantClassEntryV1Tag: {
				// Check if the Class tag is pointing to a UTF8 tag
				auto clazzEntry = reinterpret_cast<ClassConstantClassEntryV1*>(entry);
				auto nameEntry  = getEntry(clazzEntry->nameIndex);
				if (!nameEntry || nameEntry->getTag() != ClassConstantUTF8EntryV1Tag)
					return false;
				break;
			}
			case ClassConstantUTF8EntryV1Tag: break; // UTF8 tag's are always valid
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
	// Get tag and construct the specified entry
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
	// Get attribute name
	auto attributeNameEntry = constantPool.getEntry(buffer.getUI2());
	if (!attributeNameEntry || attributeNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
		// Attribute name is either invalid or is not pointing to a UTF8 tag
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidAttributeName;
		return {};
	}
	attribute.name = reinterpret_cast<ClassConstantUTF8EntryV1*>(attributeNameEntry)->string;

	// Read attribute info
	std::uint32_t attributeLength = buffer.getUI4();
	buffer.getUI1s(attribute.info, attributeLength);
	return attribute;
}

Class* loadClassV1(ClassRegistry* registry, ByteBuffer& buffer, EClassLoadStatus* loadStatus) {
	ClassConstantPoolV1 constantPool;
	// Allocate a constant pool of size 'constantPoolSize - 1'
	std::uint16_t constantPoolSize = buffer.getUI2();
	constantPool.reserve(constantPoolSize - 1);
	for (std::uint32_t i = 0; i < static_cast<std::uint32_t>(constantPoolSize) - 1; i++) {
		// Try to read a constant pool entry and add it to the constant pool
		auto entry = readConstantPoolEntryV1(buffer, loadStatus);
		if (!entry) return nullptr;
		constantPool.addEntry(entry);
	}

	// Validate the constant pool
	if (!constantPool.validate()) {
		// The constant pool is invalid
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidConstantPool;
		return nullptr;
	}

	// Read class access flags and an index into the constant pool pointing to a Class tag
	EAccessFlags accessFlags = buffer.getUI2();
	auto thisClassEntry      = constantPool.getEntry(buffer.getUI2());
	if (!thisClassEntry || thisClassEntry->getTag() != ClassConstantClassEntryV1Tag) {
		// The thisClass field is either invalid or is not pointing to a Class tag
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidThisClassEntry;
		return nullptr;
	}

	// Read super classes
	std::uint16_t superCount = buffer.getUI2();
	std::vector<std::uint16_t> supers;
	buffer.getUI2s(supers, superCount);
	// Validate the super classes
	for (auto super : supers) {
		auto superEntry = constantPool.getEntry(super);
		if (!superEntry || superEntry->getTag() != ClassConstantClassEntryV1Tag) {
			// The super class is either invalid or is not pointing to a Class tag
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidSuperClassEntry;
			return nullptr;
		}
	}

	// Read class fields
	std::uint16_t fieldCount = buffer.getUI2();
	std::vector<ClassFieldEntryV1> fields(fieldCount);
	for (std::size_t i = 0; i < fields.size(); i++) {
		auto& field = fields[i];
		// Read field access flags
		field.accessFlags = buffer.getUI2();

		// Read field name
		auto fieldNameEntry = constantPool.getEntry(buffer.getUI2());
		if (!fieldNameEntry || fieldNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			// Field name is either invalid or is not pointing to a UTF8 tag
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidFieldName;
			return nullptr;
		}
		field.name = reinterpret_cast<ClassConstantUTF8EntryV1*>(fieldNameEntry)->string;

		// Read field descriptor
		auto fieldDescriptorEntry = constantPool.getEntry(buffer.getUI2());
		if (!fieldDescriptorEntry || fieldDescriptorEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			// Field descriptor is either invalid or is not pointing to a UTF8 tag
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidFieldDescriptor;
			return nullptr;
		}
		field.descriptor = reinterpret_cast<ClassConstantUTF8EntryV1*>(fieldDescriptorEntry)->string;

		// Read field attributes
		std::uint16_t attributeCount = buffer.getUI2();
		field.attributes.resize(attributeCount);
		for (std::size_t j = 0; j < field.attributes.size(); j++) {
			// Try to read an attribute
			EClassLoadStatus loadStatus2 = EClassLoadStatus::Success;
			field.attributes[j]          = readAttributeEntryV1(buffer, constantPool, &loadStatus2);
			if (loadStatus2 != EClassLoadStatus::Success) {
				// Attribute was invalid
				if (loadStatus) *loadStatus = loadStatus2;
				return nullptr;
			}
		}
	}

	// Read class methods
	std::uint16_t methodCount = buffer.getUI2();
	std::vector<ClassMethodEntryV1> methods(methodCount);
	for (std::size_t i = 0; i < methods.size(); i++) {
		auto& method = methods[i];
		// Read method access flags
		method.accessFlags = buffer.getUI2();

		// Read method name
		auto methodNameEntry = constantPool.getEntry(buffer.getUI2());
		if (!methodNameEntry || methodNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			// Method name is either invalid or is not pointing to a UTF8 tag
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMethodName;
			return nullptr;
		}
		method.name = reinterpret_cast<ClassConstantUTF8EntryV1*>(methodNameEntry)->string;

		// Read method descriptor
		auto methodDescriptorEntry = constantPool.getEntry(buffer.getUI2());
		if (!methodDescriptorEntry || methodDescriptorEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
			// Method descriptor is either invalid or is not pointing to a UTF8 tag
			if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMethodDescriptor;
			return nullptr;
		}
		method.descriptor = reinterpret_cast<ClassConstantUTF8EntryV1*>(methodDescriptorEntry)->string;

		// Read method attributes
		std::uint16_t attributeCount = buffer.getUI2();
		method.attributes.resize(attributeCount);
		for (std::size_t j = 0; j < method.attributes.size(); j++) {
			// Try to read an attribute
			EClassLoadStatus loadStatus2 = EClassLoadStatus::Success;
			method.attributes[j]         = readAttributeEntryV1(buffer, constantPool, &loadStatus2);
			if (loadStatus2 != EClassLoadStatus::Success) {
				// Attribute was invalid
				if (loadStatus) *loadStatus = loadStatus2;
				return nullptr;
			}
		}
	}

	// Read class attributes
	std::uint16_t attributeCount = buffer.getUI2();
	std::vector<ClassAttributeEntryV1> attributes(attributeCount);
	for (std::size_t i = 0; i < attributes.size(); i++) {
		// Try to read an attribute
		EClassLoadStatus loadStatus2 = EClassLoadStatus::Success;
		attributes[i]                = readAttributeEntryV1(buffer, constantPool, &loadStatus2);
		if (loadStatus2 != EClassLoadStatus::Success) {
			// Attribute was invalid
			if (loadStatus) *loadStatus = loadStatus2;
			return nullptr;
		}
	}

	// Construct a new class from the read data
	Class* clazz       = new Class();
	clazz->accessFlags = accessFlags;

	// Get class name string
	auto thisClass     = reinterpret_cast<ClassConstantClassEntryV1*>(thisClassEntry);
	auto thisClassName = reinterpret_cast<ClassConstantUTF8EntryV1*>(constantPool.getEntry(thisClass->nameIndex));
	clazz->name        = thisClassName->string;

	// Try to load super classes
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
		auto& field = clazz->fields[i];
		auto& entry = fields[i];
		// Get field information
		field.accessFlags = entry.accessFlags;
		field.name        = entry.name;
		field.descriptor  = entry.descriptor;
		// TODO: Apply field attributes
	}

	clazz->methods.resize(methods.size());
	for (std::size_t i = 0; i < methods.size(); i++) {
		auto& method = clazz->methods[i];
		auto& entry  = methods[i];
		// Get method information
		method.accessFlags = entry.accessFlags;
		method.name        = entry.name;
		method.descriptor  = entry.descriptor;
		for (auto& attribute : entry.attributes) {
			if (attribute.name == "code") // If the attribute's name is "code" then allocate and set the code pointer of the method.
				method.allocateCode(attribute.info);
		}
	}

	// Return class
	if (loadStatus) *loadStatus = EClassLoadStatus::Success;
	return clazz;
}
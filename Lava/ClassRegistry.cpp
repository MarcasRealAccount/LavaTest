#include "ClassRegistry.h"
#include "ByteBuffer.h"

#include <cassert>
#include <cstring>

#include <set>

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
	case EClassLoadStatus::InvalidMethodRefClassName: return stream << "InvalidMethodRefClassName";
	case EClassLoadStatus::InvalidMethodRefMethodDescriptor: return stream << "InvalidMethodRefMethodDescriptor";
	}
	return stream;
}

ClassRegistry* globalClassRegistry = new ClassRegistry();

Class* ClassRegistry::newClass(const std::string& className) {
	auto itr = this->classes.find(className);
	if (itr != this->classes.end()) return nullptr;

	Class* clazz = new Class();
	clazz->name  = className;
	this->classes.insert({ className, clazz });
	return clazz;
}

void ClassRegistry::addClassPath(const std::filesystem::path classPath) {
	this->classPaths.push_back(classPath);
}

Class* ClassRegistry::getClass(const std::string& className) {
	auto itr = this->classes.find(className);
	if (itr != this->classes.end()) return itr->second;
	return nullptr;
}

Class* ClassRegistry::loadClass(const std::string& className, EClassLoadStatus* loadStatus) {
	// Look for the class in the registry
	Class* clazz = getClass(className);
	if (clazz) return clazz;

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
	case 1:
		clazz = loadClassV1(this, buffer, loadStatus);
		break;
	default:
		// Version is not one of the loadable versions
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidVersion;
		return nullptr;
	}
	if (clazz) this->classes.insert({ clazz->name, clazz });
	return clazz;
}

Class* ClassRegistry::loadClassc(const char* className, EClassLoadStatus* loadStatus) {
	return loadClass(std::string(className), loadStatus);
}

Class& ClassRegistry::loadClassError(const std::string& className) {
	EClassLoadStatus loadStatus;
	Class* clazz = loadClass(className, &loadStatus);
	if (!clazz) {
		std::ostringstream stream;
		stream << "Class could not be loaded: '" << loadStatus << "'";
		throw std::runtime_error(stream.str());
	}
	return *clazz;
}

Class& ClassRegistry::loadClassErrorc(const char* className) {
	return loadClassError(std::string(className));
}

Method& ClassRegistry::getMethodErrorc(const char* className, const char* methodName) {
	Class& clazz = loadClassErrorc(className);
	return clazz.getMethodErrorc(methodName);
}

Method& ClassRegistry::getMethodFromDescriptorErrorc(const char* className, const char* methodDescriptor) {
	Class& clazz = loadClassErrorc(className);
	return clazz.getMethodFromDescriptorErrorc(methodDescriptor);
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

struct ClassAttributeV1 {
	ClassAttributeV1(std::string&& name) : name(std::move(name)) { }

	std::string name;
};

struct ClassAttributeUnknownV1 : public ClassAttributeV1 {
	ClassAttributeUnknownV1(std::string&& name, std::vector<std::uint8_t>&& info) : info(std::move(info)), ClassAttributeV1(std::move(name)) { }

	std::vector<std::uint8_t> info;
};

struct ClassAttributeMethodCodeV1 : public ClassAttributeV1 {
	ClassAttributeMethodCodeV1(std::vector<std::uint8_t>&& code) : code(std::move(code)), ClassAttributeV1("code") { }

	std::vector<std::uint8_t> code;
};

struct ClassAttributeMethodRefV1 : public ClassAttributeV1 {
	ClassAttributeMethodRefV1(std::uint16_t classNameIndex, std::uint16_t methodDescriptorIndex, std::uint32_t byteOffset) : classNameIndex(classNameIndex), methodDescriptorIndex(methodDescriptorIndex), byteOffset(byteOffset), ClassAttributeV1("methodref") { }

	std::uint16_t classNameIndex;
	std::uint16_t methodDescriptorIndex;
	std::uint32_t byteOffset;
};

struct ClassFieldEntryV1 {
	EAccessFlags accessFlags = 0;
	std::string name;
	std::string descriptor;
	std::vector<ClassAttributeV1*> attributes;
};

struct ClassMethodEntryV1 {
	EAccessFlags accessFlags = 0;
	std::string name;
	std::string descriptor;
	std::vector<ClassAttributeV1*> attributes;
};

struct ClassMethodRefV1 {
	std::string className;
	std::string methodDescriptor;
	std::uint32_t byteOffset;
};

ClassAttributeV1* readAttributeEntryV1(ByteBuffer& buffer, ClassConstantPoolV1& constantPool, EClassLoadStatus* loadStatus) {
	// Get attribute name
	auto attributeNameEntry = constantPool.getEntry(buffer.getUI2());
	if (!attributeNameEntry || attributeNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
		// Attribute name is either invalid or is not pointing to a UTF8 tag
		if (loadStatus) *loadStatus = EClassLoadStatus::InvalidAttributeName;
		return {};
	}
	auto attributeName = reinterpret_cast<ClassConstantUTF8EntryV1*>(attributeNameEntry);
	auto& name         = attributeName->string;

	// Read attribute info
	std::uint32_t attributeLength = buffer.getUI4();

	if (name == "code") {
		std::vector<std::uint8_t> code;
		buffer.getUI1s(code, attributeLength);
		return new ClassAttributeMethodCodeV1(std::move(code));
	} else if (name == "methodref") {
		std::uint16_t classNameIndex        = buffer.getUI2();
		std::uint16_t methodDescriptorIndex = buffer.getUI2();
		std::uint32_t byteOffset            = buffer.getUI4();
		return new ClassAttributeMethodRefV1(classNameIndex, methodDescriptorIndex, byteOffset);
	} else {
		std::vector<std::uint8_t> info;
		buffer.getUI1s(info, attributeLength);
		return new ClassAttributeUnknownV1(std::string(name), std::move(info));
	}
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
	std::vector<ClassAttributeV1*> attributes(attributeCount);
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
		std::vector<ClassMethodRefV1> methodRefs;
		std::vector<std::uint8_t> code;
		for (auto& attribute : entry.attributes) {
			if (attribute->name == "code") {
				code = reinterpret_cast<ClassAttributeMethodCodeV1*>(attribute)->code;
			} else if (attribute->name == "methodref") {
				auto ref = reinterpret_cast<ClassAttributeMethodRefV1*>(attribute);
				ClassMethodRefV1 methodRef;
				methodRef.byteOffset = ref->byteOffset;

				auto methodRefClassNameEntry = constantPool.getEntry(ref->classNameIndex);
				if (!methodRefClassNameEntry || methodRefClassNameEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
					// Method-ref class is either invalid or is not pointing to a UTF8 tag
					if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMethodRefClassName;
					return nullptr;
				}
				auto methodRefClassName = reinterpret_cast<ClassConstantUTF8EntryV1*>(methodRefClassNameEntry);
				methodRef.className     = methodRefClassName->string;

				auto methodRefMethodDescriptorEntry = constantPool.getEntry(ref->methodDescriptorIndex);
				if (!methodRefMethodDescriptorEntry || methodRefMethodDescriptorEntry->getTag() != ClassConstantUTF8EntryV1Tag) {
					// Method-ref method is either invalid or is not pointing to a UTF8 tag
					if (loadStatus) *loadStatus = EClassLoadStatus::InvalidMethodRefMethodDescriptor;
					return nullptr;
				}
				auto methodRefMethodDescriptor = reinterpret_cast<ClassConstantUTF8EntryV1*>(methodRefMethodDescriptorEntry);
				methodRef.methodDescriptor     = methodRefMethodDescriptor->string;

				methodRefs.push_back(methodRef);
			}
		}

		// New way
		if (!code.empty()) {
			// Constants
			std::uintptr_t classRegistryAddr                 = reinterpret_cast<std::uintptr_t>(registry);
			std::uintptr_t getMethodFromDescriptorErrorcAddr = LavaUBCast<decltype(&ClassRegistry::getMethodFromDescriptorErrorc), std::uintptr_t>(&ClassRegistry::getMethodFromDescriptorErrorc).right;
			std::size_t pCodeOffset                          = offsetof(Method, pCode);
			bool requireLargeCall                            = pCodeOffset > 255;
			std::size_t codeLength                           = code.size();
			std::size_t getCallLength                        = requireLargeCall ? 80 : 77;
			std::size_t directCallLength                     = 12;
			// The relative call length constants are given though they aren't used atm
			std::size_t relativeGetCallLength = requireLargeCall ? 73 : 70;
			std::size_t relativeCallLength    = 5;
			std::size_t callLength            = 0;
			std::size_t dataLength            = 0;
			std::unordered_map<std::string, std::size_t> strings;
			std::set<std::string> loadedClasses;

			// Sort method refs based on their byte offset
			std::sort(methodRefs.begin(), methodRefs.end(), [](ClassMethodRefV1& lhs, ClassMethodRefV1& rhs) -> bool {
				return lhs.byteOffset < rhs.byteOffset;
			});

			// Check how much space each method ref requires to call
			for (auto& methodRef : methodRefs) {
				Class* methodRefClass = registry->getClass(methodRef.className);
				if (!methodRefClass) {
					strings.insert({ methodRef.className, 0 });
					strings.insert({ methodRef.methodDescriptor, 0 });
					callLength += getCallLength;
					continue;
				}
				loadedClasses.insert(methodRef.className);
				Method* methodRefMethod = methodRefClass->getMethodFromDescriptor(methodRef.methodDescriptor);
				if (!methodRefMethod)
					throw std::runtime_error("Method wants to invoke a nonexistant method '" + methodRef.methodDescriptor + "' in class '" + methodRef.className + "'");
				callLength += directCallLength;
			}

			// Check how much space each string requires
			for (auto& string : strings) dataLength += string.first.size() + 1;

			std::size_t dataBegin = codeLength + callLength - 1;
			// Resize the code to the new length
			code.resize(dataBegin + dataLength, 0);
			method.allocateCode(code);
			auto pCode = method.pCode;

			// Write the strings into the code
			std::size_t dataOffset = 0;
			for (auto& string : strings) {
				std::memcpy(pCode + dataBegin + dataOffset, string.first.data(), string.first.size());
				string.second = dataOffset;
				dataOffset += string.first.size() + 1;
			}

			std::size_t offset = 0;
			for (auto& methodRef : methodRefs) {
				std::size_t callBegin = offset + methodRef.byteOffset;
				// If method refers to an already loaded class optimize the call to the direct call, else use the get call
				if (loadedClasses.find(methodRef.className) != loadedClasses.end()) {
					// Move bytes after call
					std::memmove(pCode + callBegin + directCallLength, pCode + callBegin + 1, directCallLength);

					// Create the call in assembly
					Class* methodRefClass   = registry->getClass(methodRef.className);
					Method* methodRefMethod = methodRefClass->getMethodFromDescriptor(methodRef.methodDescriptor);
					ByteBuffer call;
					call.addUI1s({ 0x48, 0xB8 });                                                         // MOV RAX, ??
					call.addUI8(LavaUBCast<std::uint8_t*, std::uintptr_t>(methodRefMethod->pCode).right); // Address of method to call
					call.addUI1s({ 0xFF, 0xD0 });                                                         // CALL RAX

					// Copy the call into the code
					std::memcpy(pCode + callBegin, call.data(), directCallLength);
					offset += directCallLength;
				} else { // Use the worse in every way get call :|
					// Get string offsets
					std::int32_t classNameOffset        = static_cast<std::int32_t>(dataBegin + strings.find(methodRef.className)->second) - (callBegin + 46);
					std::int32_t methodDescriptorOffset = static_cast<std::int32_t>(dataBegin + strings.find(methodRef.methodDescriptor)->second) - (callBegin + 53);

					// Move bytes after call
					std::memmove(pCode + callBegin + getCallLength, pCode + callBegin + 1, codeLength - methodRef.byteOffset);

					// Create the call in assembly
					ByteBuffer call;
					call.addUI1s({ 0x48, 0x83, 0xEC, 0x38 });       // SUB RSP, 38h
					call.addUI1s({ 0x48, 0x89, 0x4C, 0x24, 0x20 }); // MOV [RSP + 20h], rcx
					call.addUI1s({ 0x48, 0x89, 0x54, 0x24, 0x28 }); // MOV [RSP + 28h], rdx
					call.addUI1s({ 0x4C, 0x89, 0x44, 0x24, 0x30 }); // MOV [RSP + 30h], r8
					call.addUI1s({ 0x48, 0xB8 });                   // MOV RAX, ??
					call.addUI8(getMethodFromDescriptorErrorcAddr); // getMethodFromDescriptorErrorc address
					call.addUI1s({ 0x48, 0xB9 });                   // MOV RCX, ??
					call.addUI8(classRegistryAddr);                 // classRegistry address
					call.addUI1s({ 0x48, 0x8D, 0x15 });             // LEA RDX, [REL ??]
					call.addI4(classNameOffset);                    // className offset
					call.addUI1s({ 0x4C, 0x8D, 0x05 });             // LEA R8, [REL ??]
					call.addI4(methodDescriptorOffset);             // methodDescriptor offset
					call.addUI1s({ 0xFF, 0xD0 });                   // CALL RAX
					call.addUI1s({ 0x48, 0x8B, 0x4C, 0x24, 0x20 }); // MOV rcx, [RSP + 20h]
					call.addUI1s({ 0x48, 0x8B, 0x54, 0x24, 0x28 }); // MOV rdx, [RSP + 28h]
					call.addUI1s({ 0x4C, 0x8B, 0x44, 0x24, 0x30 }); // MOV r8,  [RSP + 30h]
					call.addUI1s({ 0x48, 0x83, 0xC4, 0x38 });       // ADD RSP, 38h
					if (requireLargeCall) {
						call.addUI1s({ 0xFF, 0x90 });                         // CALL [RAX + ??]
						call.addUI4(static_cast<std::uint32_t>(pCodeOffset)); // pCode offset
					} else {
						call.addUI1s({ 0xFF, 0x50 });                        // CALL [RAX + ??]
						call.addUI1(static_cast<std::uint8_t>(pCodeOffset)); // pCode offset
					}

					// Copy the call into the code
					std::memcpy(pCode + callBegin, call.data(), getCallLength);
					offset += getCallLength;
				}
			}

			method.makeCodeExecutable();
		}
	}

	// Return class
	if (loadStatus) *loadStatus = EClassLoadStatus::Success;
	return clazz;
}
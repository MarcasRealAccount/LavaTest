#include "Class.h"

#include <cstring>

#include <sstream>

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

//-------------
// Class Flags
//-------------

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

//------------------
// Class structures
//------------------

Method::~Method() {
	if (this->allocated)
		deallocateMemory(this->pCode, this->codeLength);
}

void Method::allocateCode(std::vector<std::uint8_t>& code) {
	if (this->pCode) return;
	this->allocated  = true;
	this->codeLength = code.size();
	this->pCode      = reinterpret_cast<std::uint8_t*>(allocateReadWriteMemory(this->codeLength));
	std::memcpy(this->pCode, code.data(), this->codeLength);
}

void Method::makeCodeReadWrite() {
	makeNonExecutableMemory(this->pCode, this->codeLength);
}

void Method::makeCodeExecutable() {
	makeExecutableMemory(this->pCode, this->codeLength);
}

Method* Class::getMethod(std::string_view name) {
	for (auto& method : this->methods)
		if (method.name == name)
			return &method;
	return nullptr;
}

Method* Class::getMethodc(const char* name) {
	return getMethod(name);
}

Method& Class::getMethodError(std::string_view name) {
	for (auto& method : this->methods)
		if (method.name == name)
			return method;
	std::ostringstream stream;
	stream << "Method name '" << name << "' not found";
	throw std::runtime_error(stream.str());
}

Method& Class::getMethodErrorc(const char* name) {
	return getMethodError(name);
}

Method* Class::getMethodFromDescriptor(std::string_view descriptor) {
	for (auto& method : this->methods)
		if (method.descriptor == descriptor)
			return &method;
	return nullptr;
}

Method* Class::getMethodFromDescriptorc(const char* descriptor) {
	return getMethodFromDescriptor(descriptor);
}

Method& Class::getMethodFromDescriptorError(std::string_view descriptor) {
	for (auto& method : this->methods)
		if (method.descriptor == descriptor)
			return method;
	std::ostringstream stream;
	stream << "Method descriptor '" << descriptor << "' not found";
	throw std::runtime_error(stream.str());
}

Method& Class::getMethodFromDescriptorErrorc(const char* descriptor) {
	return getMethodFromDescriptorError(descriptor);
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
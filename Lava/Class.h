#pragma once

#include <cstdint>

#include <ostream>
#include <string>
#include <vector>

//-------------
// Class Flags
//-------------

class EAccessFlags {
public:
	constexpr EAccessFlags(std::uint16_t value) : value(value) { }

	operator std::uint16_t() const { return this->value; }

	friend constexpr bool operator==(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value == rhs.value; }
	friend constexpr bool operator!=(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value != rhs.value; }
	friend constexpr bool operator>(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value > rhs.value; }
	friend constexpr bool operator<(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value < rhs.value; }
	friend constexpr bool operator>=(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value >= rhs.value; }
	friend constexpr bool operator<=(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value <= rhs.value; }
	friend constexpr EAccessFlags operator&(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value & rhs.value; }
	friend constexpr EAccessFlags operator|(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value | rhs.value; }
	friend constexpr EAccessFlags operator^(EAccessFlags lhs, EAccessFlags rhs) { return lhs.value ^ rhs.value; }
	friend constexpr EAccessFlags operator~(EAccessFlags flags) { return ~flags.value; }
	friend constexpr EAccessFlags operator<<(EAccessFlags flags, std::size_t count) { return flags.value << count; }
	friend constexpr EAccessFlags operator>>(EAccessFlags flags, std::size_t count) { return flags.value >> count; }
	friend std::ostream& operator<<(std::ostream& stream, EAccessFlags flags);

private:
	std::uint16_t value;
};

namespace EAccessFlag {
	static constexpr EAccessFlags Public       = 0x001;
	static constexpr EAccessFlags Private      = 0x0002;
	static constexpr EAccessFlags Protected    = 0x0004;
	static constexpr EAccessFlags Static       = 0x0008;
	static constexpr EAccessFlags Final        = 0x0010;
	static constexpr EAccessFlags Super        = 0x0020;
	static constexpr EAccessFlags Synchronized = 0x0020;
	static constexpr EAccessFlags Volatile     = 0x0040;
	static constexpr EAccessFlags Bridge       = 0x0040;
	static constexpr EAccessFlags Transient    = 0x0080;
	static constexpr EAccessFlags Varargs      = 0x0080;
	static constexpr EAccessFlags Native       = 0x0100;
	static constexpr EAccessFlags Interface    = 0x0200;
	static constexpr EAccessFlags Abstract     = 0x0400;
	static constexpr EAccessFlags Strict       = 0x0800;
	static constexpr EAccessFlags Synthetic    = 0x1000;
	static constexpr EAccessFlags Annotation   = 0x2000;
	static constexpr EAccessFlags Enum         = 0x4000;
	static constexpr EAccessFlags Module       = 0x8000;
} // namespace EAccessFlag

//------------------
// Class structures
//------------------

extern "C" std::uintptr_t gInvoke(std::uint8_t* pCode, std::uint8_t arg0, std::uint8_t arg1, std::uint8_t arg2, ...);

struct Field;
struct Method;
struct Class;

struct Field {
	std::string name;
	std::string descriptor;
	EAccessFlags accessFlags = EAccessFlag::Public;
};

struct Method {
	~Method();

	std::string name;
	std::string descriptor;
	EAccessFlags accessFlags = EAccessFlag::Public;
	std::size_t codeLength   = 0;
	std::uint8_t* pCode      = nullptr;
	bool allocated           = false;

	void allocateCode(std::vector<std::uint8_t>& code);
	void makeCodeReadWrite();
	void makeCodeExecutable();
	bool isInvokable() const { return this->pCode; }
	template <class... Ts>
	std::uintptr_t invoke(Ts&&... args) { return gInvoke(this->pCode, 0, 0, 0, args...); }
};

struct Class {
	std::string name;
	EAccessFlags accessFlags = EAccessFlag::Public;
	std::vector<Class*> supers;
	std::vector<Field> fields;
	std::vector<Method> methods;
};
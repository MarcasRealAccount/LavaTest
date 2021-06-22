#pragma once

#include <cstdint>

#include <ostream>
#include <string>
#include <vector>

#if LAVA_TOOLSET_gcc && !LAVA_SYSTEM_windows
	#define LAVA_MICROSOFT_CALL_ABI __attribute__((ms_abi))
#else
	#define LAVA_MICROSOFT_CALL_ABI
#endif

#if LAVA_TOOLSET_msc
	#define LAVA_NOINLINE __declspec(noinline)
#else
	#define LAVA_NOINLINE __attribute__((noinline))
#endif

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
	static constexpr EAccessFlags Public       = 0x0001;
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

template <class Left, class Right>
union LavaUBCast {
	static_assert(!std::is_same_v<Left, Right>);

	constexpr LavaUBCast(Left left) : left(left) { }
	LavaUBCast(const LavaUBCast&) = delete;
	LavaUBCast(LavaUBCast&&)      = delete;
	LavaUBCast& operator=(const LavaUBCast&) = delete;
	LavaUBCast& operator=(LavaUBCast&&) = delete;
	~LavaUBCast() { }

	Left left;
	Right right;
};

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

	template <class T>
	void setMethod(T method) { pCode = LavaUBCast<T, std::uint8_t*>(method).right; }

	void allocateCode(std::vector<std::uint8_t>& code);
	void makeCodeReadWrite();
	void makeCodeExecutable();
	bool isInvokable() const { return this->pCode; }
	template <class R, class... Ts>
	R invoke(Ts&&... args) {
		if constexpr (std::is_void_v<R>)
			LavaUBCast<void*, void(LAVA_MICROSOFT_CALL_ABI*)(Ts...)>(this->pCode).right(args...);
		else
			return LavaUBCast<void*, R(LAVA_MICROSOFT_CALL_ABI*)(Ts...)>(this->pCode).right(args...);
	}
};

struct Class {
	std::string name;
	EAccessFlags accessFlags = EAccessFlag::Public;
	std::vector<Class*> supers;
	std::vector<Field> fields;
	std::vector<Method> methods;

	Method* getMethod(std::string_view name);
	Method* getMethodc(const char* name);
	Method& getMethodError(std::string_view name);
	Method& getMethodErrorc(const char* name);
	Method* getMethodFromDescriptor(std::string_view descriptor);
	Method* getMethodFromDescriptorc(const char* descriptor);
	Method& getMethodFromDescriptorError(std::string_view descriptor);
	Method& getMethodFromDescriptorErrorc(const char* descriptor);
};
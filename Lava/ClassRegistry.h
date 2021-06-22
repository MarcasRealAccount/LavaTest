#pragma once

#include "Class.h"

#include <cstdint>

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class EClassLoadStatus : std::uint32_t {
	Success = 0,
	FileNotFound,
	InvalidMagicNumber,
	InvalidVersion,
	InvalidConstantPool,
	InvalidConstantPoolEntry,
	InvalidThisClassEntry,
	InvalidSuperClassEntry,
	InvalidFieldName,
	InvalidFieldDescriptor,
	InvalidAttributeName,
	InvalidMethodName,
	InvalidMethodDescriptor,
	InvalidMethodRefClassName,
	InvalidMethodRefMethodDescriptor,
};

std::ostream& operator<<(std::ostream& stream, EClassLoadStatus status);

class ClassRegistry {
public:
	Class* newClass(const std::string& className);
	void addClassPath(const std::filesystem::path classPath);
	Class* getClass(const std::string& className);
	Class* loadClass(const std::string& className, EClassLoadStatus* loadStatus = nullptr);
	Class* loadClassc(const char* className, EClassLoadStatus* loadStatus = nullptr);
	Class& loadClassError(const std::string& className);
	Class& loadClassErrorc(const char* className);
	Method& getMethodErrorc(const char* className, const char* methodName);
	LAVA_MICROSOFT_CALL_ABI Method& getMethodFromDescriptorErrorc(const char* className, const char* methodDescriptor);

	auto& getClassPaths() const { return this->classPaths; }
	std::vector<Class*> getLoadedClasses() const;

private:
	std::filesystem::path findClass(std::string_view className) const;

private:
	std::vector<std::filesystem::path> classPaths;
	std::unordered_map<std::string, Class*> classes;
};

extern ClassRegistry* globalClassRegistry;
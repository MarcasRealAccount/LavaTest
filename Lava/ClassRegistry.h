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
};

std::ostream& operator<<(std::ostream& stream, EClassLoadStatus status);

class ClassRegistry {
public:
	void addClassPath(const std::filesystem::path classPath);
	Class* loadClass(const std::string& className, EClassLoadStatus* loadStatus = nullptr);

	auto& getClassPaths() const { return this->classPaths; }
	std::vector<Class*> getLoadedClasses() const;

private:
	std::filesystem::path findClass(std::string_view className) const;

private:
	std::vector<std::filesystem::path> classPaths;
	std::unordered_map<std::string, Class*> classes;
};
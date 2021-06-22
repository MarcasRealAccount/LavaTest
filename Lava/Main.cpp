#include "ClassRegistry.h"

#include <filesystem>
#include <iostream>

void debugPrintClass(Class& clazz) {
	std::cout << "Class '" << clazz.name << "'\n";
	std::cout << "\tAccess Flags: '" << clazz.accessFlags << "'\n";
	for (std::size_t i = 0; i < clazz.supers.size(); i++)
		std::cout << "\tSuper '" << clazz.supers[i]->name << "'\n";
	for (std::size_t i = 0; i < clazz.fields.size(); i++) {
		auto& field = clazz.fields[i];
		std::cout << "\tField '" << field.name << "'\n";
		std::cout << "\t\tDescriptor: '" << field.descriptor << "'\n";
		std::cout << "\t\tAccessFlags: '" << field.accessFlags << "'\n";
	}
	for (std::size_t i = 0; i < clazz.methods.size(); i++) {
		auto& method = clazz.methods[i];
		std::cout << "\tMethod '" << method.name << "'\n";
		std::cout << "\t\tDescriptor: '" << method.descriptor << "'\n";
		std::cout << "\t\tAccessFlags: '" << method.accessFlags << "'\n";
		std::cout << "\t\tCode:" << std::hex << std::uppercase << "\n\t\t\t";
		std::size_t column = 0;
		for (std::size_t j = 0; j < method.codeLength; j++) {
			if (column > 0) {
				if ((column % 8) == 0)
					std::cout << "  ";
				else
					std::cout << " ";
			}
			std::uint64_t byte = static_cast<uint64_t>(method.pCode[j]);
			if (byte <= 0xF) std::cout << '0';
			std::cout << byte;
			column++;
			if (column >= 16) {
				std::cout << "\n\t\t\t";
				column = 0;
			}
		}
		std::cout << std::dec << std::nouppercase << "\n";
	}
}

LAVA_MICROSOFT_CALL_ABI std::uint64_t returnFirstArg(std::uint64_t arg) {
	return arg + 6;
}

int main() {
	// Add current working directory to the class paths
	globalClassRegistry->addClassPath(".");

#if 1
	// Construct a new class before starting app
	auto otherClazz = globalClassRegistry->newClass("Other");
	Method otherClazzL;
	otherClazzL.name       = "L";
	otherClazzL.descriptor = "L";
	otherClazzL.setMethod(&returnFirstArg);
	otherClazz->methods.push_back(otherClazzL);
#endif

	// Load class "Test" from the "Test.lclass" file in the "Run" directory
	auto& clazz = globalClassRegistry->loadClassErrorc("Test");
	// Debug print class information
	debugPrintClass(clazz);
	// Invoke the method 'P' in the class
	auto& method = clazz.getMethodFromDescriptorErrorc("P");

	std::uint64_t result = method.invoke<int, std::uint64_t, std::uint64_t, std::uint64_t>(1, 2, 3);
	// Print the return value from the method
	std::cout << "Returned: " << std::hex << std::uppercase << result << std::dec << std::nouppercase << "\n";
}
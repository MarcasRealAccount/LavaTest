#include "ClassRegistry.h"

#include <filesystem>
#include <iostream>

void debugPrintClass(Class* clazz) {
	std::cout << "Class '" << clazz->name << "'\n";
	std::cout << "\tAccess Flags: '" << clazz->accessFlags << "'\n";
	for (std::size_t i = 0; i < clazz->supers.size(); i++)
		std::cout << "\tSuper '" << clazz->supers[i]->name << "'\n";
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
		for (std::size_t j = 0; j < method.codeLength; j++) {
			if (column > 0) {
				if ((column % 8) == 0)
					std::cout << "  ";
				else
					std::cout << " ";
			}
			std::cout << static_cast<uint64_t>(method.pCode[j]);
			column++;
			if (column >= 16) {
				std::cout << "\n\t\t\t";
				column = 0;
			}
		}
		std::cout << std::dec << std::nouppercase << "\n";
	}
}

int main() {
	ClassRegistry classRegistry;
	// Add current working directory to the class paths
	classRegistry.addClassPath(".");
	// Load class "Test" from the "Test.lclass" file in the "Run" directory
	EClassLoadStatus loadStatus;
	Class* clazz = classRegistry.loadClass("Test", &loadStatus);
	if (!clazz) {
		std::cerr << "Class could not be loaded '" << loadStatus << "'!" << std::endl;
		return EXIT_FAILURE;
	}
	// Debug print class information
	debugPrintClass(clazz);
	// Invoke the first method in the class
	int result = clazz->methods[0].invoke<char, char, char>(1, 2, 3);
	// Print the return value from the method
	std::cout << "Returned: " << std::hex << std::uppercase << result << std::dec << std::nouppercase << "\n";
}
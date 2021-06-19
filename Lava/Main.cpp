#include "ClassRegistry.h"

#include <filesystem>
#include <iostream>

void debugPrintClass(Class* clazz) {
	std::cout << "Class '" << clazz->name << "'\n";
	std::cout << "\tAccess Flags: '" << clazz->accessFlags << "'\n";
	std::cout << "\tSupers:\n";
	for (std::size_t i = 0; i < clazz->supers.size(); i++)
		std::cout << "\t\t'" << clazz->supers[i]->name << "'\n";
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

template <class R, class... Params>
union FunctionCast {
	using FuncType = R (*)(Params...);

	FunctionCast(FuncType func) : func(func) { }
	FunctionCast(void* ptr) : ptr(ptr) { }

	FuncType func;
	void* ptr;
};

int main() {
	ClassRegistry classRegistry;
	classRegistry.addClassPath(".");
	EClassLoadStatus loadStatus;
	Class* clazz = classRegistry.loadClass("Test", &loadStatus);
	if (!clazz) {
		std::cerr << "Class could not be loaded '" << loadStatus << "'!" << std::endl;
		return EXIT_FAILURE;
	}
	debugPrintClass(clazz);
	int result = FunctionCast<int>(clazz->methods[0].pCode).func();
	std::cout << "Returned: " << std::hex << std::uppercase << result << std::dec << std::nouppercase << "\n";
}
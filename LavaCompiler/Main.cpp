#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

struct Field {
	std::uint16_t accessFlag = 0x0001;
	std::string name;
	std::string descriptor;
};

struct MethodRef {
	std::string className;
	std::string methodDescriptor;
	std::uint32_t codeOffset;
};

struct Method {
	std::uint16_t accessFlag = 0x0001;
	std::string name;
	std::string descriptor;
	std::vector<std::uint8_t> code;
	std::vector<MethodRef> methodRefs;
};

int main(int argc, const char** argv) {
	std::filesystem::path file;
	if (argc < 2) {
		std::cerr << "Missing output file argument, using default 'Test.lclass'" << std::endl;
		file = "Test.lclass";
	} else {
		file = argv[1];
	}

	std::ofstream lclassFile(file, std::ios::binary);
	if (lclassFile) {
		std::uint32_t magic       = 0x484F544C;
		std::uint16_t version     = 1;
		std::uint16_t accessFlags = 0x0001;
		std::string className;
		std::vector<std::string> superClassNames;
		std::vector<Field> fields;
		std::vector<Method> methods;

		char ws;
		std::cout << "Class name: ";
		std::cin >> className;
		std::cin.ignore(1000, '\n');
		while (true) {
			std::string superClass;
			std::cout << "Super class name: ";
			std::getline(std::cin, superClass);
			if (superClass.empty()) break;
			superClassNames.push_back(std::move(superClass.substr(0, superClass.find_first_of(' '))));
		}
		while (true) {
			Field field;
			std::cout << "Field name: ";
			std::getline(std::cin, field.name);
			if (field.name.empty()) break;
			field.name = field.name.substr(0, field.name.find_first_of(' '));
			std::cout << "Field descriptor: ";
			std::cin >> field.descriptor;
			std::cin.ignore(1000, '\n');
			fields.push_back(std::move(field));
		}
		while (true) {
			Method method;
			std::cout << "Method name: ";
			std::getline(std::cin, method.name);
			if (method.name.empty()) break;
			method.name = method.name.substr(0, method.name.find_first_of(' '));
			std::cout << "Method descriptor: ";
			std::cin >> method.descriptor;
			std::cin.ignore(1000, '\n');
			std::cout << "Method code: ";
			while (true) {
				std::string bytes;
				std::getline(std::cin, bytes);
				if (bytes.empty()) break;
				std::string_view bytesView = bytes;
				std::size_t offset         = 0;
				while (offset < bytes.size()) {
					std::size_t end           = bytesView.find_first_of(' ', offset);
					std::string_view byteView = bytesView.substr(offset, end - offset);
					if ((byteView.size() % 2) == 1) {
						std::cout << "Warning you passed an odd number of nibbles (4 bits), skipping '" << byteView << "'" << std::endl;
						offset = bytesView.find_first_not_of(' ', end);
						continue;
					}

					std::vector<std::uint8_t> code;
					bool skip = false;
					for (std::size_t i = 0; i < byteView.size(); i += 2) {
						char hn = byteView[i];
						char ln = byteView[i + 1];
						std::uint8_t hv;
						std::uint8_t lv;
						if (hn >= '0' && hn <= '9')
							hv = hn - '0';
						else if (hn >= 'a' && hn <= 'f')
							hv = 10 + hn - 'a';
						else if (hn >= 'A' && hn <= 'F')
							hv = 10 + hn - 'A';
						else {
							std::cout << "Warning Nibble " << i << " is not one of (0-9, a-f, A-F), skipping '" << byteView << "'" << std::endl;
							skip = true;
							break;
						}
						if (ln >= '0' && ln <= '9')
							lv = ln - '0';
						else if (ln >= 'a' && ln <= 'f')
							lv = 10 + ln - 'a';
						else if (ln >= 'A' && ln <= 'F')
							lv = 10 + ln - 'A';
						else {
							std::cout << "Warning Nibble " << (i + 1) << " is not one of (0-9, a-f, A-F), skipping '" << byteView << "'" << std::endl;
							skip = true;
							break;
						}
						std::uint8_t byte = hv << 4 | lv;
						code.push_back(byte);
					}

					if (skip) {
						offset = bytesView.find_first_not_of(' ', end);
						continue;
					}
					method.code.insert(method.code.end(), code.begin(), code.end());
					offset = bytesView.find_first_not_of(' ', end);
				}
			}

			while (true) {
				MethodRef methodRef;
				std::cout << "Method ref class name: ";
				std::getline(std::cin, methodRef.className);
				if (methodRef.className.empty()) break;
				std::cout << "Method ref method descriptor: ";
				std::cin >> methodRef.methodDescriptor;
				std::cout << "Method ref code offset: ";
				std::cin >> methodRef.codeOffset;
				std::cin.ignore(1000, '\n');
				method.methodRefs.push_back(std::move(methodRef));
			}

			methods.push_back(std::move(method));
		}

		lclassFile.write(reinterpret_cast<char*>(&magic), 4);
		lclassFile.write(reinterpret_cast<char*>(&version), 2);
		std::unordered_map<std::string, std::uint16_t> stringToConstantPoolIndex;
		std::unordered_map<std::string, std::uint16_t> classToConstantPoolIndex;
		classToConstantPoolIndex.insert({ className, 0 });
		stringToConstantPoolIndex.insert({ className, 0 });
		for (auto& superClass : superClassNames) {
			classToConstantPoolIndex.insert({ superClass, 0 });
			stringToConstantPoolIndex.insert({ superClass, 0 });
		}
		for (auto& field : fields) {
			stringToConstantPoolIndex.insert({ field.name, 0 });
			stringToConstantPoolIndex.insert({ field.descriptor, 0 });
		}
		for (auto& method : methods) {
			stringToConstantPoolIndex.insert({ method.name, 0 });
			stringToConstantPoolIndex.insert({ method.descriptor, 0 });
			if (!method.code.empty()) stringToConstantPoolIndex.insert({ "code", 0 });
			if (!method.methodRefs.empty()) stringToConstantPoolIndex.insert({ "methodref", 0 });
			for (auto& methodRef : method.methodRefs) {
				stringToConstantPoolIndex.insert({ methodRef.className, 0 });
				stringToConstantPoolIndex.insert({ methodRef.methodDescriptor, 0 });
			}
		}
		std::uint16_t constantPoolCount = stringToConstantPoolIndex.size() + classToConstantPoolIndex.size() + 1;
		lclassFile.write(reinterpret_cast<char*>(&constantPoolCount), 2);
		std::uint16_t currentConstantPoolIndex = 1;
		for (auto& stringConstant : stringToConstantPoolIndex) {
			std::uint8_t tag = 2;
			lclassFile.write(reinterpret_cast<char*>(&tag), 1);
			std::uint32_t stringLength = static_cast<std::uint32_t>(stringConstant.first.size());
			lclassFile.write(reinterpret_cast<char*>(&stringLength), 4);
			lclassFile.write(stringConstant.first.data(), stringLength);
			stringConstant.second = currentConstantPoolIndex++;
		}
		for (auto& classConstant : classToConstantPoolIndex) {
			std::uint8_t tag = 1;
			lclassFile.write(reinterpret_cast<char*>(&tag), 1);
			auto itr = stringToConstantPoolIndex.find(classConstant.first);
			if (itr == stringToConstantPoolIndex.end()) {
				std::cerr << "An unexpected error occured: Class name was not found in the constant pool, please try again." << std::endl;
				lclassFile.close();
				return EXIT_FAILURE;
			}
			std::uint16_t index = itr->second;
			lclassFile.write(reinterpret_cast<char*>(&index), 2);
			classConstant.second = currentConstantPoolIndex++;
		}
		lclassFile.write(reinterpret_cast<char*>(&accessFlags), 2);
		{
			auto itr = classToConstantPoolIndex.find(className);
			if (itr == classToConstantPoolIndex.end()) {
				std::cerr << "An unexpected error occured: Class name was not found in the constant pool, please try again." << std::endl;
				lclassFile.close();
				return EXIT_FAILURE;
			}
			std::uint16_t index = itr->second;
			lclassFile.write(reinterpret_cast<char*>(&index), 2);
		}
		std::uint16_t superCount = static_cast<std::uint16_t>(superClassNames.size());
		lclassFile.write(reinterpret_cast<char*>(&superCount), 2);
		for (auto& superClass : superClassNames) {
			auto itr = classToConstantPoolIndex.find(superClass);
			if (itr == classToConstantPoolIndex.end()) {
				std::cerr << "An unexpected error occured: Super class name was not found in the constant pool, please try again." << std::endl;
				lclassFile.close();
				return EXIT_FAILURE;
			}
			std::uint16_t index = itr->second;
			lclassFile.write(reinterpret_cast<char*>(&index), 2);
		}
		std::uint16_t fieldCount = static_cast<std::uint16_t>(fields.size());
		lclassFile.write(reinterpret_cast<char*>(&fieldCount), 2);
		for (auto& field : fields) {
			lclassFile.write(reinterpret_cast<char*>(&field.accessFlag), 2);
			{
				auto itr = stringToConstantPoolIndex.find(field.name);
				if (itr == stringToConstantPoolIndex.end()) {
					std::cerr << "An unexpected error occured: Field name was not found in the constant pool, please try again." << std::endl;
					lclassFile.close();
					return EXIT_FAILURE;
				}
				std::uint16_t index = itr->second;
				lclassFile.write(reinterpret_cast<char*>(&index), 2);
			}
			{
				auto itr = stringToConstantPoolIndex.find(field.descriptor);
				if (itr == stringToConstantPoolIndex.end()) {
					std::cerr << "An unexpected error occured: Field descriptor was not found in the constant pool, please try again." << std::endl;
					lclassFile.close();
					return EXIT_FAILURE;
				}
				std::uint16_t index = itr->second;
				lclassFile.write(reinterpret_cast<char*>(&index), 2);
			}
			std::uint16_t attributeCount = 0;
			lclassFile.write(reinterpret_cast<char*>(attributeCount), 2);
		}
		std::uint16_t methodCount = static_cast<std::uint16_t>(methods.size());
		lclassFile.write(reinterpret_cast<char*>(&methodCount), 2);
		for (auto& method : methods) {
			lclassFile.write(reinterpret_cast<char*>(&method.accessFlag), 2);
			{
				auto itr = stringToConstantPoolIndex.find(method.name);
				if (itr == stringToConstantPoolIndex.end()) {
					std::cerr << "An unexpected error occured: Method name was not found in the constant pool, please try again." << std::endl;
					lclassFile.close();
					return EXIT_FAILURE;
				}
				std::uint16_t index = itr->second;
				lclassFile.write(reinterpret_cast<char*>(&index), 2);
			}
			{
				auto itr = stringToConstantPoolIndex.find(method.descriptor);
				if (itr == stringToConstantPoolIndex.end()) {
					std::cerr << "An unexpected error occured: Method descriptor was not found in the constant pool, please try again." << std::endl;
					lclassFile.close();
					return EXIT_FAILURE;
				}
				std::uint16_t index = itr->second;
				lclassFile.write(reinterpret_cast<char*>(&index), 2);
			}
			std::uint16_t attributeCount = (!method.code.empty()) + (!method.methodRefs.empty());
			lclassFile.write(reinterpret_cast<char*>(&attributeCount), 2);
			if (!method.code.empty()) {
				{
					auto itr = stringToConstantPoolIndex.find("code");
					if (itr == stringToConstantPoolIndex.end()) {
						std::cerr << "An unexpected error occured: \"code\" was not found in the constant pool, please try again." << std::endl;
						lclassFile.close();
						return EXIT_FAILURE;
					}
					std::uint16_t index = itr->second;
					lclassFile.write(reinterpret_cast<char*>(&index), 2);
				}
				std::uint32_t codeLength = static_cast<std::uint32_t>(method.code.size());
				lclassFile.write(reinterpret_cast<char*>(&codeLength), 4);
				lclassFile.write(reinterpret_cast<char*>(method.code.data()), codeLength);
			}
			for (auto& methodRef : method.methodRefs) {
				{
					auto itr = stringToConstantPoolIndex.find("methodref");
					if (itr == stringToConstantPoolIndex.end()) {
						std::cerr << "An unexpected error occured: \"methodref\" was not found in the constant pool, please try again." << std::endl;
						lclassFile.close();
						return EXIT_FAILURE;
					}
					std::uint16_t index = itr->second;
					lclassFile.write(reinterpret_cast<char*>(&index), 2);
				}
				std::uint32_t attributeLength = 8;
				lclassFile.write(reinterpret_cast<char*>(&attributeLength), 4);
				{
					auto itr = stringToConstantPoolIndex.find(methodRef.className);
					if (itr == stringToConstantPoolIndex.end()) {
						std::cerr << "An unexpected error occured: Method ref class name was not found in the constant pool, please try again." << std::endl;
						lclassFile.close();
						return EXIT_FAILURE;
					}
					std::uint16_t index = itr->second;
					lclassFile.write(reinterpret_cast<char*>(&index), 2);
				}
				{
					auto itr = stringToConstantPoolIndex.find(methodRef.methodDescriptor);
					if (itr == stringToConstantPoolIndex.end()) {
						std::cerr << "An unexpected error occured: Method ref method descriptor was not found in the constant pool, please try again." << std::endl;
						lclassFile.close();
						return EXIT_FAILURE;
					}
					std::uint16_t index = itr->second;
					lclassFile.write(reinterpret_cast<char*>(&index), 2);
				}
				lclassFile.write(reinterpret_cast<char*>(&methodRef.codeOffset), 4);
			}
		}

		lclassFile.close();
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}
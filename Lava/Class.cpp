#include "Class.h"

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
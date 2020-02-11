#pragma once

#include <string>
#include <VMUtils/json_binding.hpp>

VM_BEGIN_MODULE( vol )

using namespace std;

struct RawDescriptor : vm::json::Serializable<RawDescriptor>
{
	VM_JSON_FIELD( string, path );
	VM_JSON_FIELD( int, x );
	VM_JSON_FIELD( int, y );
	VM_JSON_FIELD( int, z );
};

VM_END_MODULE()

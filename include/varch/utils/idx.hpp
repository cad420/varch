#pragma once

#include <cstdint>
#include <VMUtils/fmt.hpp>
#include <VMUtils/attributes.hpp>
#include <VMUtils/modules.hpp>
#include <VMUtils/json_binding.hpp>

VM_BEGIN_MODULE( vol )

using namespace std;

#pragma pack( push )
#pragma pack( 4 )

VM_EXPORT
{
	struct Idx
	{
		VM_DEFINE_ATTRIBUTE( uint32_t, x );
		VM_DEFINE_ATTRIBUTE( uint32_t, y );
		VM_DEFINE_ATTRIBUTE( uint32_t, z );

		uint64_t total() const { return (uint64_t)x * y * z; }

		bool operator<( Idx const &other ) const
		{
			return x < other.x ||
				   x == other.x && ( y < other.y ||
									 y == other.y && z < other.z );
		}
		bool operator==( Idx const &other ) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
		bool operator!=( Idx const &other ) const
		{
			return !( *this == other );
		}

		friend ostream &operator<<( ostream &os, Idx const &_ )
		{
			vm::fprint( os, "{}", make_tuple( _.x, _.y, _.z ) );
			return os;
		}
	};

	inline void to_json( nlohmann::json & j, Idx const &idx )
	{
		j = { idx.x, idx.y, idx.z };
	}

	inline void from_json( nlohmann::json const &j, Idx &idx )
	{
		idx.x = j[ 0 ].get<std::size_t>();
		idx.y = j[ 1 ].get<std::size_t>();
		idx.z = j[ 2 ].get<std::size_t>();
	}
}

#pragma pack( pop )

VM_END_MODULE()

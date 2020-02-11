#pragma once

#include <fstream>
#include <vector>
#include <VMat/numeric.h>
#include <VMat/geometry.h>
#include <VMUtils/modules.hpp>
#include <VMUtils/attributes.hpp>
#include <varch/utils/io.hpp>
#include <varch/utils/common.hpp>

VM_BEGIN_MODULE( vol )

VM_EXPORT
{
	struct Thumbnail
	{
		Thumbnail( vol::Idx const &dim ) :
		  dim( dim ),
		  buf( dim.total(), 0.f )
		{
		}
		Thumbnail( std::string const &file_name )
		{
			std::ifstream is( file_name, std::ios::ate | std::ios::binary );
			auto len = is.tellg();
			vol::StreamReader reader( is, 0, len );

			reader.read_typed( dim );
			reader.read_typed( buf );
		}
		Thumbnail( vol::Reader &reader )
		{
			reader.read_typed( dim );
			reader.read_typed( buf );
		}

	public:
		float *data() { return buf.data(); }
		float const *data() const { return buf.data(); }

		float &operator[]( vol::Idx const &idx )
		{
			return buf[ idx.z * dim.x * dim.y +
						idx.y * dim.x +
						idx.x ];
		}

		float const &operator[]( vol::Idx const &idx ) const
		{
			return buf[ idx.z * dim.x * dim.y +
						idx.y * dim.x +
						idx.x ];
		}

		template <typename F>
		void iterate_3d( F const &f ) const
		{
			vol::Idx idx;
			for ( idx.z = 0; idx.z != dim.z; ++idx.z ) {
				for ( idx.y = 0; idx.y != dim.y; ++idx.y ) {
					for ( idx.x = 0; idx.x != dim.x; ++idx.x ) {
						f( idx );
					}
				}
			}
		}

		// std::vector<vol::Idx> present_block_idxs() const
		// {
		// 	std::vector<vol::Idx> block_idxs;
		// 	iterate_3d(
		// 	  [&]( vol::Idx const &idx ) {
		// 		  if ( !( *this )[ idx ].chebyshev ) {
		// 			  block_idxs.emplace_back( idx );
		// 		  }
		// 	  } );
		// 	return block_idxs;
		// }

		void dump( vol::Writer &writer ) const
		{
			writer.write_typed( dim );
			writer.write_typed( buf );
		}

	public:
		VM_DEFINE_ATTRIBUTE( vol::Idx, dim );

	private:
		std::vector<float> buf;
	};
}

VM_END_MODULE()

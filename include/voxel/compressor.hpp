#pragma once

#include <map>

#include <video/compressor.hpp>
#include <unbounded_io.hpp>
#include <voxel/internal/util.hpp>

namespace vol
{
VM_BEGIN_MODULE( voxel )

using namespace std;

VM_EXPORT
{
	template <typename Voxel = char>
	struct Compressor final : vm::NoCopy, vm::NoMove
	{
		Compressor( Idx const &block_dim,
					UnboundedWriter &writer,
					video::Compressor &video ) :
		  block_dim( block_dim ),
		  block_len( block_dim.total() * sizeof( Voxel ) / sizeof( char ) ),
		  writer( writer ),
		  video( video )
		{
		}
		~Compressor()
		{
			writer.seek( fend );
			auto a = writer.tell();
			uint64_t n = 0;
			for ( auto &e : index ) {
				++n;
				auto &idx = e.first;
				auto &blk = e.second;
				idx.write_to( writer );
				blk.write_to( writer );
			}

			auto meta = CompressMeta{}
						  .set_block_count( n )
						  .set_index_offset( a );
			meta.write_to( writer );
		}

		void put( Idx const &idx, Reader &reader )
		{
			if ( index.find( idx ) != index.end() ) {
				return;
			}
			auto a = writer.tell();
			video.compress( reader, writer );
			auto b = writer.tell();
			fend = std::max( b, fend );
			auto len = b - a;
			index[ idx ] = BlockIndex{}
							 .set_offset( a )
							 .set_len( len );
		}
		void put( Idx const &idx, istream &is, size_t offset = 0 )
		{
			StreamReader reader( is, offset, block_len );
			put( idx, reader );
		}
		void put( Idx const &idx, Voxel const *block )
		{
			auto ptr = reinterpret_cast<char const *>( block );
			SliceReader reader( ptr, 0, block_len );
			put( idx, reader );
		}

	private:
		Idx block_dim;
		size_t block_len;
		size_t fend = 0;
		UnboundedWriter &writer;
		video::Compressor &video;
		std::map<Idx, BlockIndex> index;
	};
}

VM_END_MODULE()

}  // namespace vol

#pragma once

#include <thread>
#include <VMUtils/nonnull.hpp>
#include <VMUtils/timer.hpp>
#include <VMUtils/fmt.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include <VMat/geometry.h>
#include <vocomp/index/index.hpp>
#include <koi.hpp>
#include <impl/fs.hpp>
#include <utils/slab.hpp>
#include "block_mempool.hpp"

namespace vol
{
VM_BEGIN_MODULE( refine )

using namespace std;

struct BlockInputStreamerOptions
{
	VM_DEFINE_ATTRIBUTE( string, path );
	VM_DEFINE_ATTRIBUTE( index::Idx, raw_size );
	VM_DEFINE_ATTRIBUTE( index::Idx, sub_region_begin );
	VM_DEFINE_ATTRIBUTE( index::Idx, sub_region_size );
	VM_DEFINE_ATTRIBUTE( size_t, voxel_size );
	VM_DEFINE_ATTRIBUTE( size_t, block_size );
	VM_DEFINE_ATTRIBUTE( size_t, padding ) = 0;
	VM_DEFINE_ATTRIBUTE( size_t, io_queue_size ) = thread::hardware_concurrency();
};

struct BlockInputStreamerImpl : vm::NoCopy, vm::NoMove
{
	struct Spill
	{
		ysl::Vec3i m;
		ysl::Vec3i p;
	};

	BlockInputStreamerImpl( BlockInputStreamerOptions const &opts ) :
	  input( koi::fs::File::open_sync( opts.path ) ),
	  opts( opts ),
	  block_inner( opts.block_size - 2 * opts.padding ),
	  dim( index::Idx{}
			 .set_x( ysl::RoundUpDivide( opts.sub_region_size.x, block_inner ) )
			 .set_y( ysl::RoundUpDivide( opts.sub_region_size.y, block_inner ) )
			 .set_z( ysl::RoundUpDivide( opts.sub_region_size.z, block_inner ) ) )
	{
		vm::println( "{} {}", opts.sub_region_size, block_inner );
		auto px = int( dim.x * block_inner - opts.sub_region_size.x );
		auto py = int( dim.y * block_inner - opts.sub_region_size.y );
		auto pz = int( dim.z * block_inner - opts.sub_region_size.z );
		auto padding = int( opts.padding );
		for ( int x = 0; x != 4; ++x ) {
			for ( int y = 0; y != 4; ++y ) {
				for ( int z = 0; z != 4; ++z ) {
					auto &_ = spill[ x ][ y ][ z ];
					_.m = ysl::Vec3i{
						x & 1 ? 0 : -padding,
						y & 1 ? 0 : -padding,
						z & 1 ? 0 : -padding
					};
					_.p = ysl::Vec3i{
						x & 2 ? -px : padding,
						y & 2 ? -py : padding,
						z & 2 ? -pz : padding
					} - _.m;
				}
			}
		}
	}

	auto stream( BlockMempool &pool )
	{
		using O = BlockMempool::Block;
		return koi::future::stream_poll_fn<O>(
		  [&, buffer = koi::utils::Option<O>(),
		   sink = koi::future::sink(),
		   idx = ysl::Size3{ 0, 0, 0 }]( koi::utils::Option<O> &_ ) mutable -> koi::future::StreamState {
			  // batch = 1 x buffer
			  if ( !buffer.has_value() ) {
				  buffer = pool.alloc();
				  if ( !buffer.has_value() ) {
					  return koi::future::StreamState::Pending;
				  }
			  } else {
				  switch ( sink.poll() ) {
				  case koi::future::PollState::Ok:
					  _ = std::move( buffer );
					  buffer = koi::utils::None{};
					  return koi::future::StreamState::Yield;
				  case koi::future::PollState::Pending:
					  return koi::future::StreamState::Pending;
				  default:
					  throw std::runtime_error(
						vm::fmt( "failed to process block {}", idx ) );
				  }
			  }
			  // for ( auto i = 0; i != io_queue_size; ++i ) {
			  // 	  auto one_query_stream = koi::future::stream_poll_fn<bool>(
			  // 		[&, i, idx]( koi::utils::Option<bool> &_ ) mutable -> koi::future::StreamState {
			  // 			// read file here
			  // 			input.read()
			  // 			koi::spawn();
			  // 			return koi::future::StreamState::Done;
			  // 		} );
			  // 	  koi::spawn(
			  // 		std::move( one_query_stream )
			  // 		  .drain()
			  // 		  // .take_while(
			  // 		  // 	[]( bool ok ) mutable -> bool { return ok; } )
			  // 		  .gather( sink.handle() ) );
			  // }
			  auto &dxyz = calc_dxyz( idx );
			  auto raw_begin = ysl::Size3{
				  opts.sub_region_begin.x + dxyz.m.x,
				  opts.sub_region_begin.y + dxyz.m.y,
				  opts.sub_region_begin.z + dxyz.m.z
			  } + block_inner * idx;
			  auto raw_size = ysl::Size3{
				  block_inner + dxyz.p.x,
				  block_inner + dxyz.p.y,
				  block_inner + dxyz.p.z
			  };
			  vm::println( "{}: {} {}, {} {}", idx, dxyz.m, dxyz.p, raw_begin, raw_size );
			  vm::println( "{}", this_thread::get_id() );

			  for ( auto i = 0; i != 1; ++i ) {
				  using F = decltype( input.read( nullptr, 0 ) );
				  auto read_task = koi::future::poll_fn<void>(
					[&, stride = opts.io_queue_size, i,
					 raw_size = opts.raw_size,
					 block_size = opts.block_size,
					 raw_begin = raw_begin,
					 z = size_t( i ), y = size_t( 0 ),
					 buf = buffer.value().get(),
					 fut = vm::Option<F>()]( auto &_ ) mutable -> koi::future::PollState {
						if ( fut.has_value() ) {
							switch ( auto poll = fut.value().poll() ) {
							default: return poll;
							case koi::future::PollState::Ok:
								if ( !( y = ( y + 1 ) % raw_size.y ) ) {
									if ( ( z += stride ) >= raw_size.z ) {
										return koi::future::PollState::Ok;
									}
								}
							}
						}
						auto raw_pt = raw_begin + ysl::Size3{ 0, y, z };
						auto nraw = raw_pt.x +
									raw_pt.y * raw_size.x +
									raw_pt.z * raw_size.x * raw_size.y;
						auto n = y * block_size +
								 z * block_size * block_size;
						// vm::println( "{} {}", n, nraw );
						fut = input.read( buf + n, raw_size.x, nraw );
						return koi::future::PollState::Pending;
					} );
				  koi::spawn(
					std::move( read_task )
					  .gather( sink.handle() ) );
				  // for ( size_t z = )
			  }

			  if ( !( idx.x = ( idx.x + 1 ) % dim.x ) ) {
				  if ( !( idx.y = ( idx.y + 1 ) % dim.y ) ) {
					  if ( !( idx.z = ( idx.z + 1 ) % dim.z ) ) {
						  return koi::future::StreamState::Done;
					  }
				  }
			  }
			  return koi::future::StreamState::Pending;
		  } );
	}

	Spill const &calc_dxyz( ysl::Size3 const &idx )
	{
		int x = 0, y = 0, z = 0;
		if ( idx.x == 0 ) x |= 1;
		if ( idx.y == 0 ) y |= 1;
		if ( idx.z == 0 ) z |= 1;
		if ( idx.x == dim.x - 1 ) x |= 2;
		if ( idx.y == dim.y - 1 ) y |= 2;
		if ( idx.z == dim.z - 1 ) z |= 2;
		return spill[ x ][ y ][ z ];
	}

	koi::fs::File input;

	const BlockInputStreamerOptions opts;
	const size_t block_inner;
	const index::Idx dim;
	Spill spill[ 4 ][ 4 ][ 4 ];
};

struct BlockInputStreamer final
{
	BlockInputStreamer( BlockInputStreamerOptions const &_ ) :
	  _( new BlockInputStreamerImpl( _ ) )
	{
	}

	auto stream( BlockMempool &pool )
	{
		return _->stream( pool );
	}

private:
	vm::Arc<BlockInputStreamerImpl> _;
};

VM_END_MODULE()

}  // namespace vol

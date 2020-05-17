#include <fstream>
#include <string>
#include <algorithm>
#include <glog/logging.h>
#include <VMUtils/cmdline.hpp>
#include <varch/package_meta.hpp>
#include <varch/unarchive/unarchiver.hpp>
#include "tool_utils.hpp"

using namespace std;
using namespace cppfs;
using namespace vol;

int main( int argc, char **argv )
{
	cmdline::parser a;
	a.add<string>( "dir", 'd', "target directory" );

	a.parse_check( argc, argv );

	auto dir = FilePath( a.get<string>( "dir" ) );
	ensure_dir( dir.resolved() );

	struct SampleLvlLess
	{
		bool operator()( const Idx &a, const Idx &b ) const
		{
			return a.total() > b.total();
		}
	};
	map<Idx, MtSampleLevel, SampleLvlLess> lvls;
	map<string, Idx> iarch;

	int block_size = -1;
	int padding = -1;
	auto files = fs::open( dir.resolved() ).listFiles();
	for ( auto &file : files ) {
		auto filename = dir.resolve( file ).resolved();
		auto fh = fs::open( filename );
		if ( fh.isFile() ) {
			auto path = FilePath( filename );
			if ( path.extension() == ".h264" ) {
				auto fip = fh.createInputStream( ios::binary | ios::ate );
				auto len = fip->tellg();
				StreamReader reader( *fip, 0, len );
				Unarchiver uu( reader );
				if ( block_size != -1 ) {
					if ( block_size != uu.block_size() ) {
					    LOG( FATAL ) << "conflicting block size";
					}
				} else {
					block_size = uu.block_size();
				}
				if ( padding != -1 ) {
					if ( padding != uu.padding() ) {
					    LOG( FATAL ) << "conflicting padding";
					}
				} else {
					padding = uu.padding();
				}
				auto lvl = MtSampleLevel{}
				              .set_raw( uu.raw() )
							  .set_dim( uu.dim() )
							  .set_adjusted( uu.adjusted() )
							  .set_size_mb( uu.adjusted().total() / 1024 / 1024 )
							  .set_path( file );
				lvls.emplace( uu.raw(), lvl );
				iarch[ file ] = uu.raw();
			}
		}
	}
	
	for ( auto &file : files ) {
		auto filename = dir.resolve( file ).resolved();
		auto fh = fs::open( filename );
		if ( fh.isFile() ) {
			auto path = FilePath( filename );
			auto sig = FilePath( path.extension().substr( 1 ) );
			if ( sig.extension() == ".thumb" ) {
				auto &idx = iarch[ path.baseName() + ".h264" ];
				auto &thumbs = lvls[ idx ].thumbnails;
				thumbs[ sig.baseName() ] = file;
			}
		}
	}

	auto meta = PackageMeta{}
		.set_block_size( block_size )
		.set_padding( padding );

	auto raw = lvls.begin()->first;
	for ( auto &e : lvls ) {
		e.second.raw = e.first;
		int lvl = log2( raw.x / e.second.raw.x );
		if ( lvl != meta.sample_levels.size() ) {
			LOG( FATAL ) << "lvl != meta.sample_levels.size()";
		}
		meta.sample_levels.emplace_back( std::move( e.second ) );
	}

	ofstream os( dir.resolve( "package_meta.json" ).resolved() );
	auto writer = vm::json::Writer{}
					.set_indent( 2 );
	writer.write( os, meta );
}

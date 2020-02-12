#include <fstream>
#include <string>
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

	PackageMeta meta;

	struct SampleLvlLess
	{
		bool operator()( const Idx &a, const Idx &b ) const
		{
			return a.total() < b.total();
		}
	};
	map<Idx, MtSampleLevel, SampleLvlLess> lvls;
	map<string, pair<Idx, size_t>> iarch;

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
				auto arch = MtArchive{}
							  .set_dim( uu.dim() )
							  .set_adjusted( uu.adjusted() )
							  .set_block_size( uu.block_size() )
							  .set_padding( uu.padding() )
							  .set_path( file );
				auto &archs = lvls[ uu.raw() ].archives;
				iarch[ file ] = { uu.raw(), archs.size() };
				archs.emplace_back( std::move( arch ) );
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
				auto arch = path.baseName() + ".h264";
				// vm::println( "{}", arch );
				auto &idx = iarch[ arch ];
				auto thumb = MtThumbnail{}
							   .set_value( file )
							   .set_path( arch );
				auto &thumbs = lvls[ idx.first ].archives[ idx.second ].thumbnails;
				thumbs.emplace_back( std::move( thumb ) );
			}
		}
	}

	for ( auto &e : lvls ) {
		e.second.raw = e.first;
		meta.sample_levels.emplace_back( std::move( e.second ) );
	}

	ofstream os( dir.resolve( "package_meta.json" ).resolved() );
	auto writer = vm::json::Writer{}
					.set_pretty( true )
					.set_indent( 2 );
	writer.write( os, meta );
}

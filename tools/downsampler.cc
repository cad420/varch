#include <fstream>
#include <VMat/geometry.h>
#include <VMat/numeric.h>
#include <VMFoundation/rawreader.h>
#include <VMUtils/cmdline.hpp>
// #include <varch/unarchive/unarchiver.hpp>
#include <varch/utils/io.hpp>
#include "tool_utils.hpp"

using namespace std;
using namespace cppfs;

int main( int argc, char **argv )
{
	cmdline::parser a;
	a.add<string>( "in", 'i', ".raw input filename", true );
	a.add<string>( "out", 'o', ".raw output directory", false, "." );
	a.add<int>( "x", 'x', "raw.x", true );
	a.add<int>( "y", 'y', "raw.y", true );
	a.add<int>( "z", 'z', "raw.z", true );
	a.add<int>( "s", 's', "downsample factor in log", true, 3 );

	a.parse_check( argc, argv );

	auto in = FilePath( a.get<string>( "in" ) );
	ensure_file( in.resolved() );
	auto out = FilePath( a.get<string>( "out" ) );
	mkdir_p( out.resolved() );
	auto dim = vm::Vec3i(
	  a.get<int>( "x" ),
	  a.get<int>( "y" ),
	  a.get<int>( "z" ) );
	int s = 1 << a.get<int>( "s" );

	auto dim_after = vm::Size3(
	  vm::RoundUpDivide( dim.x, s ),
	  vm::RoundUpDivide( dim.y, s ),
	  vm::RoundUpDivide( dim.z, s ) );

	vm::RawReader input( in.resolved(), vm::Size3( dim ), sizeof( char ) );
	auto out_path = out.resolve( FilePath( vm::fmt( "{}_x{}.raw", in.baseName(), s ) ) ).resolved();
	ofstream os( out_path, ios::binary );
	vector<unsigned char> buf( dim.x * dim.y * s );

	for ( int t = 0; t < dim.z; t += s ) {
		auto dz = min( s, dim.z - t );

		auto start = vm::Vec3i( 0, 0, t );
		auto size = vm::Size3( dim.x, dim.y, dz );

		vector<unsigned char> res;

		input.readRegion( start, size, buf.data() );
		for ( int y = 0; y < dim.y; y += s ) {
			for ( int x = 0; x < dim.x; x += s ) {
				unsigned char v = 0;
				for ( int k = 0; k < dz; ++k ) {
					for ( int j = y; j < y + s && j < dim.y; ++j ) {
						for ( int i = x; i < x + s && s < dim.x; ++i ) {
							v = std::max( buf[ k * dim.x + dim.y + j * dim.x + i ], v );
						}
					}
				}
				res.emplace_back( vm::Clamp( round( v ), 0, 255 ) );
			}
		}

		os.write( reinterpret_cast<char *>( res.data() ), res.size() );
	}

	vm::println( "{}", dim_after );
}

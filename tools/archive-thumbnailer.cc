#include <string>
#include <set>
#include <VMUtils/cmdline.hpp>
#include <VMUtils/timer.hpp>
#include <varch/utils/unbounded_io.hpp>
#include <varch/unarchive/unarchiver.hpp>
#include <varch/unarchive/statistics.hpp>
#include <varch/thumbnail.hpp>
#include "tool_utils.hpp"

using namespace std;
using namespace vol;
using namespace cppfs;

enum Value
{
	Mean,
	Chebyshev
};

struct OptReader
{
	set<Value> operator()( const string &str )
	{
		set<Value> ret;
		string val;
		size_t beg = 0, end;
		do {
			end = str.find_first_of( ',', beg );
			if ( end == str.npos ) {
				val = str.substr( beg );
			} else {
				val = str.substr( beg, end - beg );
				beg = end + 1;
			}
			if ( val == "chebyshev" ) {
				ret.emplace( Chebyshev );
			} else if ( val == "mean" ) {
				ret.emplace( Mean );
			}
		} while ( end != str.npos );
		return ret;
	}
};

ostream &operator<<( ostream &os, set<Value> const &ss )
{
	os << "{";
	for ( auto &e : ss ) {
		os << e << ", ";
	}
	os << "}";
	return os;
}

void chebyshev( Thumbnail &dst, Thumbnail const &value, float threshold )
{
	auto &dim = dst.dim;
	auto maxd = std::max( dim.x, std::max( dim.y, dim.z ) );
	static vm::Vec3i d14[] = {
		{ -1, 0, 0 },
		{ 1, 0, 0 },
		{ 0, -1, 0 },
		{ 0, 1, 0 },
		{ 0, 0, -1 },
		{ 0, 0, 1 },
		{ -1, -1, -1 },
		{ -1, 1, -1 },
		{ 1, 1, -1 },
		{ 1, -1, -1 },
		{ -1, -1, 1 },
		{ -1, 1, 1 },
		{ 1, 1, 1 },
		{ 1, -1, 1 },
	};
	dst.iterate_3d(
	  [&]( vol::Idx const &idx ) {
		  if ( value[ idx ] <= threshold ) {
			  dst[ idx ] = maxd;
		  }
	  } );
	bool update;
	do {
		update = false;
		dst.iterate_3d(
		  [&]( vol::Idx const &idx ) {
			  if ( value[ idx ] <= threshold ) {
				  vm::Vec3i u( idx.x, idx.y, idx.z );
				  for ( int i = 0; i != 14; ++i ) {
					  auto v = u + d14[ i ];
					  float d0;
					  if ( v.x < 0 || v.y < 0 || v.z < 0 ||
						   v.x >= dim.x || v.y >= dim.y || v.z >= dim.z ) {
						  d0 = maxd;
					  } else {
						  auto idx = Idx{}
									   .set_x( v.x )
									   .set_y( v.y )
									   .set_z( v.z );
						  d0 = dst[ idx ] + 1.f;
					  }
					  if ( d0 < dst[ idx ] ) {
						  dst[ idx ] = d0;
						  update = true;
					  }
				  }
			  }
		  } );
	} while ( update );
}

void write_thumb( Thumbnail const &thumb, string const &name )
{
	ofstream os( name + ".thumb" );
	UnboundedStreamWriter writer( os );
	thumb.dump( writer );
}

int main( int argc, char **argv )
{
	cmdline::parser a;
	a.add<string>( "in", 'i', "input archive file name", true );
	a.add<string>( "out", 'o', "output directory", false, "." );
	a.add<set<Value>>( "values", 'x', "compute value field names", true, {}, OptReader() );

	a.parse_check( argc, argv );

	auto in = FilePath( a.get<string>( "in" ) );
	ensure_file( in.path() );
	auto out = FilePath( a.get<string>( "out" ) );
	mkdir_p( out.path() );
	auto val = a.get<set<Value>>( "values" );

	ifstream is( in.path(), ios::ate | ios::binary );
	auto len = is.tellg();
	StreamReader reader( is, 0, len );
	Unarchiver unarchiver( reader );

	const double max_t = 8, avg_t = 1e-3;

	Statistics stats;
	StatisticsCollector collector( unarchiver );

	shared_ptr<Thumbnail> mean;
	if ( val.count( Mean ) || val.count( Chebyshev ) ) {
		mean.reset( new Thumbnail( unarchiver.dim() ) );
		vm::Timer::Scoped timer(
		  [&]( auto dt ) {
			  vm::println( "mean compute time: {}", dt.ms() );
		  } );
		mean->iterate_3d(
		  [&]( Idx const &idx ) {
			  collector.compute_into( idx, stats );
			  if ( stats.src.max < max_t && stats.src.avg < avg_t ) {
				  ( *mean )[ idx ] = 0;
				  // continue;
			  } else {
				  ( *mean )[ idx ] = stats.src.avg;
			  }
		  } );

		write_thumb( *mean,
					 out.resolve( FilePath( in.baseName() + ".mean" ) ).path() );
	}

	if ( val.count( Chebyshev ) ) {
		Thumbnail chebyshev_thumb( unarchiver.dim() );
		vm::Timer::Scoped timer(
		  [&]( auto dt ) {
			  vm::println( "chebyshev compute time: {}", dt.ms() );
		  } );
		chebyshev( chebyshev_thumb, *mean, 0 );

		write_thumb( chebyshev_thumb,
					 out.resolve( FilePath( in.baseName() + ".chebyshev" ) ).path() );
	}
}

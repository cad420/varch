#include <iostream>
#include <string>
#include <cppfs/fs.h>
#include <cppfs/FileHandle.h>
#include <cppfs/FilePath.h>
#include <VMUtils/cmdline.hpp>
#include <varch/archive/archiver.hpp>
#include "tool_utils.hpp"

#ifdef WIN32
#include <windows.h>
unsigned long long get_system_memory()
{
	MEMORYSTATUSEX status;
	status.dwLength = sizeof( status );
	GlobalMemoryStatusEx( &status );
	return status.ullTotalPhys;
}
#else
#include <unistd.h>
unsigned long long get_system_memory()
{
	long pages = sysconf( _SC_PHYS_PAGES );
	long page_size = sysconf( _SC_PAGE_SIZE );
	return pages * page_size;
}
#endif

using namespace std;
using namespace vol;
using namespace cppfs;

int main( int argc, char **argv )
{
	auto system_memory_gb = get_system_memory() / 1024 /*kb*/ / 1024 /*mb*/ / 1024 /*gb*/;

	cmdline::parser a;
	a.add<string>( "in", 'i', ".raw input filename", true );
	a.add<string>( "out", 'o', "output directory", false, "." );
	a.add<int>( "x", 'x', "raw.x", true );
	a.add<int>( "y", 'y', "raw.y", true );
	a.add<int>( "z", 'z', "raw.z", true );
	a.add<size_t>( "memlimit", 'm', "maximum memory limit in gb", false, system_memory_gb / 2 );
	a.add<int>( "padding", 'p', "block padding", false, 2, cmdline::oneof<int>( 0, 1, 2 ) );
	a.add<int>( "side", 's', "block size in log(voxel)", false, 6, cmdline::oneof<int>( 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 ) );
	a.add<string>( "device", 'd', "video compression device: default/cuda/cpu", false, "default", cmdline::oneof<string>( "default", "cuda", "cpu" ) );

	//cout<<a.usage();
	a.parse_check( argc, argv );

	auto in = FilePath( a.get<string>( "in" ) );
	ensure_file( in.resolved() );
	auto out = FilePath( a.get<string>( "out" ) );
	mkdir_p( out.resolved() );
	auto x = a.get<int>( "x" );
	auto y = a.get<int>( "y" );
	auto z = a.get<int>( "z" );
	auto padding = a.get<int>( "padding" );
	auto log = a.get<int>( "side" );
	auto dev = a.get<string>( "device" );
	auto mem = a.get<size_t>( "memlimit" );

	try {
		auto opts = ArchiverOptions{}
					  .set_x( x )
					  .set_y( y )
					  .set_z( z )
					  .set_log_block_size( log )
					  .set_padding( padding )
					  .set_suggest_mem_gb( mem )
					  .set_input( in.resolved() );

		auto &compress_opts = opts.compress_opts;
		compress_opts = EncodeOptions{}
						  .set_encode_preset( EncodePreset::Default )
						  .set_width( 1024 )
						  .set_height( 1024 )
						  .set_batch_frames( 16 );
		if ( dev == "cuda" ) {
			compress_opts.set_device( ComputeDevice::Cuda );
		} else if ( dev == "cpu" ) {
			compress_opts.set_device( ComputeDevice::Cpu );
		} else if ( dev == "default" ) {
			compress_opts.set_device( ComputeDevice::Default );
		} else {
			throw std::logic_error( vm::fmt( "unknown device: {}", dev ) );
		}

		auto short_name = vm::fmt( "{}_{}p{}.h264", in.baseName(), 1 << log, padding );
		opts.set_output( out.resolve( FilePath( short_name ) ).resolved() );

		{
			Archiver archiver( opts );

			archiver.convert();

			vm::println( "written to {}", opts.output );
		}
	} catch ( exception &e ) {
		vm::eprintln( "{}", e.what() );
	}
}

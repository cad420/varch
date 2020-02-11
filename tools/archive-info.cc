#include <fstream>
#include <VMUtils/cmdline.hpp>
#include <VMUtils/fmt.hpp>
#include <varch/unarchive/unarchiver.hpp>
#include <varch/utils/io.hpp>
#include "tool_utils.hpp"

using namespace std;

int main( int argc, char **argv )
{
	cmdline::parser a;
	a.add<string>( "in", 'i', "input compressed filename", true );

	a.parse_check( argc, argv );

	try {
		auto in = a.get<string>( "in" );
		ensure_file( in );

		ifstream is( in, std::ios::ate | std::ios::binary );
		auto len = is.tellg();
		vol::StreamReader reader( is, 0, len );
		vol::Unarchiver e( reader );

		vm::println( "{>16}: {}", "Size", e.raw() );
		vm::println( "{>16}: {}", "Padded Size", e.adjusted() );
		vm::println( "{>16}: {}", "Grid Size", e.dim() );
		vm::println( "{>16}: {} = 2^{}", "Block Size", e.block_size(), e.log_block_size() );
		vm::println( "{>16}: {}", "Padding", e.padding() );

	} catch ( exception &e ) {
		vm::eprintln( "{}", e.what() );
	}
}

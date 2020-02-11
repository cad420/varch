#pragma once

#include <cstdlib>
#include <cppfs/fs.h>
#include <cppfs/FileHandle.h>
#include <cppfs/FilePath.h>
#include <VMUtils/fmt.hpp>

inline void ensure_file( std::string const &path_v )
{
	auto path = cppfs::fs::open( path_v );
	if ( !path.exists() ) {
		vm::eprintln( "the specified path '{}' doesn't exist",
					  path_v );
		exit( 1 );
	} else if ( path.isFile() ) {
		vm::eprintln( "the specified path '{}' is not a file",
					  path_v );
		exit( 1 );
	}
}

inline void mkdir_p( std::string const &path_v )
{
	auto out_dir = cppfs::fs::open( path_v );
	if ( !out_dir.exists() ) {
		out_dir.createDirectory();
	} else if ( !out_dir.isDirectory() ) {
		vm::eprintln( "the specified path '{}' is not a directory",
					  path_v );
		exit( 1 );
	}
}

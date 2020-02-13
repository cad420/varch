#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <VMUtils/modules.hpp>
#include <varch/utils/idx.hpp>

VM_BEGIN_MODULE( vol )

using MtDict = std::unordered_map<std::string, std::string>;

VM_EXPORT
{
	// struct MtThumbnail : vm::json::Serializable<MtThumbnail>
	// {
	// 	VM_JSON_FIELD( std::string, value );
	// 	VM_JSON_FIELD( std::string, path );
	// };

	struct MtArchive : vm::json::Serializable<MtArchive>
	{
		VM_JSON_FIELD( Idx, dim );
		VM_JSON_FIELD( Idx, adjusted );
		VM_JSON_FIELD( uint32_t, block_size );
		VM_JSON_FIELD( uint32_t, padding );
		VM_JSON_FIELD( std::string, path );
		VM_JSON_FIELD( MtDict, thumbnails );
	};

	struct MtSampleLevel : vm::json::Serializable<MtSampleLevel>
	{
		VM_JSON_FIELD( Idx, raw );
		VM_JSON_FIELD( std::vector<MtArchive>, archives );
	};

	struct PackageMeta : vm::json::Serializable<PackageMeta>
	{
		VM_JSON_FIELD( std::vector<MtSampleLevel>, sample_levels );
	};
}

VM_END_MODULE()

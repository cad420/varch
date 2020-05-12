#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <VMUtils/fmt.hpp>
#include <VMUtils/modules.hpp>
#include <varch/utils/idx.hpp>

VM_BEGIN_MODULE( vol )

VM_EXPORT
{
	struct MtSampleLevel : vm::json::Serializable<MtSampleLevel>
	{
		using MtStringDict = std::unordered_map<std::string, std::string>;
		
		VM_JSON_FIELD( Idx, raw );
		VM_JSON_FIELD( Idx, dim );
		VM_JSON_FIELD( Idx, adjusted );
		VM_JSON_FIELD( uint32_t, size_mb );		
		VM_JSON_FIELD( std::string, path );
		VM_JSON_FIELD( MtStringDict, thumbnails );
	};

	struct PackageMeta : vm::json::Serializable<PackageMeta>
	{
		using MtSampleLevels = std::vector<MtSampleLevel>;

		VM_JSON_FIELD( uint32_t, block_size );
		VM_JSON_FIELD( uint32_t, padding );
		VM_JSON_FIELD( MtSampleLevels, sample_levels );
	};
}

VM_END_MODULE()

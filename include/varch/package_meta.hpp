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
	// struct MtThumbnail : vm::json::Serializable<MtThumbnail>
	// {
	// 	VM_JSON_FIELD( std::string, value );
	// 	VM_JSON_FIELD( std::string, path );
	// };

	struct MtArchive : vm::json::Serializable<MtArchive>
	{
		using MtStringDict = std::unordered_map<std::string, std::string>;

		VM_JSON_FIELD( Idx, dim );
		VM_JSON_FIELD( Idx, adjusted );
		VM_JSON_FIELD( uint32_t, block_size );
		VM_JSON_FIELD( uint32_t, padding );
		VM_JSON_FIELD( std::string, path );
		VM_JSON_FIELD( MtStringDict, thumbnails );
	};

	struct MtSampleLevel : vm::json::Serializable<MtSampleLevel>
	{
		VM_JSON_FIELD( Idx, raw );
		VM_JSON_FIELD( std::vector<MtArchive>, archives );
	};

	struct PackageMeta : vm::json::Serializable<PackageMeta>
	{
		using MtSampleLevelDict = std::map<int, MtSampleLevel>;

		VM_JSON_FIELD( MtSampleLevelDict, sample_levels );
	};

	void to_json( nlohmann::json & j, PackageMeta::MtSampleLevelDict const &e )
	{
		j = nlohmann::json::object();
		for ( auto &p : e ) {
			j[ vm::fmt( "{}", p.first ) ] = p.second;
		}
	}

	void from_json( nlohmann::json const &j, PackageMeta::MtSampleLevelDict &e )
	{
		e.clear();
		for ( auto it = j.begin(); it != j.end(); ++it ) {
			e[ std::atoi( it.key().data() ) ] = it.value();
		}
	}
}

VM_END_MODULE()

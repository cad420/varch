#pragma once

#include <VMUtils/concepts.hpp>
#include <VMUtils/nonnull.hpp>
#include <VMUtils/option.hpp>
#include <utils/slab.hpp>
#include <utils/option.hpp>
// #include <sync/mutex.hpp>

namespace vol
{
VM_BEGIN_MODULE( refine )

struct BlockMempool final : vm::NoCopy, vm::NoMove
{
	struct Block final
	{
		char *get() const { return _->data; }
		std::size_t size() const { return _->pool->block_size; }

	private:
		Block( BlockMempool &pool ) :
		  _( new Inner )
		{
			_->pool = &pool;
		}

		struct Inner : vm::NoCopy, vm::NoMove
		{
			~Inner()
			{
				pool->_.remove( idx );
			}

			char *data;
			std::size_t idx;
			BlockMempool *pool;
		};
		vm::Arc<Inner> _;
		friend struct BlockMempool;
	};

	BlockMempool( size_t block_size, size_t block_num ) :
	  _( block_num ),
	  buf( block_size * block_num ),
	  block_size( block_size ),
	  block_num( block_num )
	{
	}

	/* thread unsafe */
	koi::utils::Option<Block> alloc()
	{
		if ( _.size() < block_num ) {
			Block blk( *this );
			blk._->idx = _.emplace();
			blk._->data = buf.data() + block_size * blk._->idx;
			return koi::utils::Option<Block>( koi::utils::New{}, blk );
		} else {
			return koi::utils::None{};
		}
	}

	/* thread unsafe */
	bool full() const
	{
		return _.size() >= block_num;
	}

private:
	// koi::sync::Mutex<koi::utils::Slab<>> _;
	koi::utils::Slab<> _;
	vector<char> buf;
	std::size_t block_size, block_num;
	friend struct Block;
};

VM_END_MODULE()

}  // namespace vol

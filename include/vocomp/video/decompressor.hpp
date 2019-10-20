#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include "method.hpp"
#include "../io.hpp"
#include <cudafx/stream.hpp>
#include <cudafx/misc.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/array.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

struct DecompressorImpl;

VM_EXPORT
{
	struct Decompressor final : Pipe, vm::NoCopy
	{
		Decompressor( EncodeMethod encode );
		~Decompressor();

		void decompress( Reader &reader, Writer &writer );
		std::future<cufx::Result> decompress( Reader &reader, cufx::GlobalMemory const &swap,
												cufx::Array3D<unsigned char> const &dst,
												cufx::Extent const &dim,
												cufx::Stream const &stream = cufx::Stream::null() );

		void transfer( Reader &reader, Writer &writer ) override
		{
			decompress( reader, writer );
		}

	private:
		vm::Box<DecompressorImpl> _;
	};
}

VM_END_MODULE()

}  // namespace vol

#include <vocomp/video/decompressor.hpp>

#include <nvcodec/NvDecoder.h>
#include <cudafx/driver/context.hpp>
#include <cudafx/memory.hpp>
#include <cudafx/transfer.hpp>
#include <cudafx/array.hpp>

namespace vol
{
VM_BEGIN_MODULE( video )

struct DecompressorImpl final : vm::NoCopy, vm::NoMove
{
	DecompressorImpl( EncodeMethod encode )
	{
		dec.reset(
		  new NvDecoder(
			ctx,
			[&] {
				switch ( encode ) {
				case EncodeMethod::H264: return cudaVideoCodec_H264;
				case EncodeMethod::HEVC: return cudaVideoCodec_HEVC;
				default: throw std::runtime_error( "unknown encoding" );
				}
			}() ) );
	}

	std::vector<uint32_t> &decode_header( Reader &reader )
	{
		uint32_t nframes;
		reader.read( reinterpret_cast<char *>( &nframes ), sizeof( nframes ) );
		thread_local std::vector<uint32_t> frame_len;
		frame_len.resize( nframes + 1 );
		reader.read( reinterpret_cast<char *>( frame_len.data() ),
					 sizeof( uint32_t ) * nframes );
		frame_len[ nframes ] = 0;
		return frame_len;
	}

	char *get_packet( uint32_t len )
	{
		thread_local auto _ = std::vector<char>( 1024 );
		if ( len > _.size() ) {
			_.resize( len );
		}
		return _.data();
	}

	void decompress( Reader &reader, Writer &writer )
	{
		// dec->on_picture_display.with(
		//   on_picture_display,
		//   [&] {
		auto &frame_len = decode_header( reader );
		for ( auto &len : frame_len ) {
			uint8_t **ppframe;
			int nframedec;

			auto packet = get_packet( len );
			reader.read( packet, len );
			dec->Decode( reinterpret_cast<uint8_t *>( packet ), len, &ppframe, &nframedec );

			for ( int i = 0; i < nframedec; i++ ) {
				writer.write( reinterpret_cast<char *>( ppframe[ i ] ), dec->GetFrameSize() );
			}
		}
		// } );
	}

	void decompress( Reader &reader, cufx::MemoryView1D<unsigned char> const &swap )
	{
		// assert(swap.size() == block_size)
		auto stream = cufx::Stream{};
		auto dp_swap = swap.ptr();
		auto lock = stream.lock();
		auto on_picture_display = [&]( CUdeviceptr dp_src, unsigned src_pitch ) {
			cuCtxPushCurrent( ctx );  //ck

			CUDA_MEMCPY2D m = { 0 };
			m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
			m.srcPitch = src_pitch;
			m.dstMemoryType = CU_MEMORYTYPE_DEVICE;
			m.dstPitch = m.WidthInBytes = dec->m_nWidth * dec->m_nBPP;

			m.srcDevice = dp_src;
			m.dstDevice = (CUdeviceptr)dp_swap;
			m.Height = dec->m_nLumaHeight;
			cuMemcpy2DAsync( &m, lock.get() );  //ck
			dp_swap += m.WidthInBytes * m.Height;

			m.srcDevice = dp_src + m.srcPitch * dec->m_nSurfaceHeight;
			m.dstDevice = (CUdeviceptr)dp_swap;
			m.Height = dec->m_nChromaHeight;
			cuMemcpy2DAsync( &m, lock.get() );  //ck
			dp_swap += m.WidthInBytes * m.Height;

			cuStreamSynchronize( lock.get() );  //ck
			cuCtxPopCurrent( nullptr );			//ck
		};
		dec->on_picture_display.with(
		  on_picture_display, [&] {
			  auto &frame_len = decode_header( reader );
			  for ( auto &len : frame_len ) {
				  auto packet = get_packet( len );
				  reader.read( packet, len );
				  dec->Decode( reinterpret_cast<uint8_t *>( packet ), len, nullptr, nullptr );
			  }
		  } );
	}

private:
	cufx::drv::Context ctx = 0;
	std::unique_ptr<NvDecoder> dec;
};

VM_EXPORT
{
	Decompressor::Decompressor( EncodeMethod encode ) :
	  _( new DecompressorImpl( encode ) )
	{
	}
	Decompressor::~Decompressor()
	{
	}
	void Decompressor::decompress( Reader & reader, Writer & writer )
	{
		_->decompress( reader, writer );
	}

	void Decompressor::decompress( Reader & reader, cufx::MemoryView1D<unsigned char> const &swap )
	{
		return _->decompress( reader, swap );
	}
}

VM_END_MODULE()

}  // namespace vol

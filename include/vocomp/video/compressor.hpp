#pragma once

#include <VMUtils/nonnull.hpp>
#include <VMUtils/concepts.hpp>
#include <VMUtils/attributes.hpp>
#include "method.hpp"
#include "../io.hpp"

namespace vol
{
VM_BEGIN_MODULE( video )

struct CompressorImpl;

VM_EXPORT
{
	enum class EncodePreset : char
	{
		Default,
		HP,
		HQ,
		BD,
		LowLatencyDefault,
		LowLatencyHQ,
		LowLatencyHP,
		LosslessDefault,
		LosslessHP
	};
	enum class PixelFormat : char
	{
		IYUV,
		YV12,
		NV12,
		YUV42010Bit,
		YUV444,
		YUV44410Bit,
		ARGB,
		ARGB10,
		AYUV,
		ABGR,
		ABGR10
	};
	enum class CompressDevice
	{
		Cuda,	/* cuda sdk required */
		Graphics /* D3D9 for windows and GL for linux */
	};

	struct CompressOptions
	{
		CompressOptions();

		VM_DEFINE_ATTRIBUTE( CompressDevice, device );
		VM_DEFINE_ATTRIBUTE( EncodeMethod, encode_method ) = EncodeMethod::H264;
		VM_DEFINE_ATTRIBUTE( EncodePreset, encode_preset ) = EncodePreset::Default;
		VM_DEFINE_ATTRIBUTE( unsigned, width ) = 1024;
		VM_DEFINE_ATTRIBUTE( unsigned, height ) = 1024;
		VM_DEFINE_ATTRIBUTE( unsigned, batch_frames ) = 64;
		VM_DEFINE_ATTRIBUTE( PixelFormat, pixel_format ) = PixelFormat::IYUV;
	};

	struct Compressor final : vm::NoCopy
	{
		Compressor( Writer &out, CompressOptions const &_ = CompressOptions{} );
		~Compressor();

		void accept( vm::Arc<Reader> &&reader );
		void flush( bool wait = false );
		void wait();
		uint32_t frame_size() const;
		std::vector<uint32_t> const &frame_len() const;
		uint32_t frame_count() const { return nframes; }

	private:
		vm::Box<CompressorImpl> _;
		uint32_t nframes = 0;
	};
}

VM_END_MODULE()

}  // namespace vol

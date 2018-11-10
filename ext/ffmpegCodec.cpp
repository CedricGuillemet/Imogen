#include "ffmpegCodec.h"

#include <iostream>

namespace FFMPEGCodec
{

	int(*Log)(const char *szFormat, ...) = Log;
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#  define av_frame_alloc  avcodec_alloc_frame
	//Ancient versions used av_freep
#  if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54,59,100)
#    define av_frame_free   av_freep
#  else
#    define av_frame_free   avcodec_free_frame
#  endif
#endif

	// PIX_FMT was renamed to AV_PIX_FMT on this version
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51,74,100)
#  define AVPixelFormat PixelFormat
#  define AV_PIX_FMT_RGB24 PIX_FMT_RGB24
#  define AV_PIX_FMT_RGB48 PIX_FMT_RGB48
#  define AV_PIX_FMT_YUVJ420P PIX_FMT_YUVJ420P
#  define AV_PIX_FMT_YUVJ422P PIX_FMT_YUVJ422P
#  define AV_PIX_FMT_YUVJ440P PIX_FMT_YUVJ440P
#  define AV_PIX_FMT_YUVJ444P PIX_FMT_YUVJ444P
#  define AV_PIX_FMT_YUV420P  PIX_FMT_YUV420P
#  define AV_PIX_FMT_YUV422P  PIX_FMT_YUV422P
#  define AV_PIX_FMT_YUV440P  PIX_FMT_YUV440P
#  define AV_PIX_FMT_YUV444P  PIX_FMT_YUV444P
#endif

	// r_frame_rate deprecated in ffmpeg
	// see ffmpeg commit #aba232c for details
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(52,42,0)
#  define r_frame_rate avg_frame_rate
#endif

	// Changes for ffmpeg 3.0
#define USE_FFMPEG_3_0 (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,0))

#if USE_FFMPEG_3_0
#  define av_free_packet av_packet_unref
#  define avpicture_get_size(fmt,w,h) av_image_get_buffer_size(fmt,w,h,1)

	inline int avpicture_fill(AVPicture *picture, uint8_t *ptr,
		enum AVPixelFormat pix_fmt, int width, int height)
	{
		AVFrame *frame = reinterpret_cast<AVFrame *>(picture);
		return av_image_fill_arrays(frame->data, frame->linesize,
			ptr, pix_fmt, width, height, 1);
	}
#endif

	// Changes for ffmpeg 3.1
#define USE_FFMPEG_3_1 (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57, 48, 101))

#if USE_FFMPEG_3_1
	// AVStream::codec was changed to AVStream::codecpar
#  define stream_codec(ix) m_format_context->streams[(ix)]->codecpar
	// avcodec_decode_video2 was deprecated.
	// This now works by sending `avpkt` to the decoder, which buffers the
	// decoded image in `avctx`. Then `avcodec_receive_frame` will copy the
	// frame to `picture`.
	inline int receive_frame(AVCodecContext *avctx, AVFrame *picture,
		AVPacket *avpkt)
	{
		int ret;

		ret = avcodec_send_packet(avctx, avpkt);

		if (ret < 0)
			return 0;

		ret = avcodec_receive_frame(avctx, picture);

		if (ret < 0)
			return 0;

		return 1;
	}
#else
#  define stream_codec(ix) m_format_context->streams[(ix)]->codec
	inline int receive_frame(AVCodecContext *avctx, AVFrame *picture,
		AVPacket *avpkt)
	{
		int ret;
		avcodec_decode_video2(avctx, picture, &ret, avpkt);
		return ret;
	}
#endif


	// Changes for ffmpeg 4.0
#define USE_FFMPEG_4_0 (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(58, 18, 100))

#if (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(56, 56, 100))
#  define CODEC_CAP_DELAY AV_CODEC_CAP_DELAY
#endif

	void RegisterAll()
	{
		av_register_all();
	}

	bool Decoder::Open(const std::string &name)
	{
		const char *file_name = name.c_str();
		av_log_set_level(AV_LOG_FATAL);
		if (avformat_open_input(&m_format_context, file_name, NULL, NULL) != 0) // avformat_open_input allocs format_context
		{
			//error("\"%s\" could not open input", file_name);
			return false;
		}
		if (avformat_find_stream_info(m_format_context, NULL) < 0)
		{
			//error("\"%s\" could not find stream info", file_name);
			return false;
		}
		m_video_stream = -1;
		for (unsigned int i = 0; i < m_format_context->nb_streams; i++)
		{
			if (stream_codec(i)->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				if (m_video_stream < 0)
				{
					m_video_stream = i;
				}
				m_video_indexes.push_back(i); // needed for later use
				break;
			}
		}
		if (m_video_stream == -1) {
			//error("\"%s\" could not find a valid videostream", file_name);
			return false;
		}

		// codec context for videostream
#if USE_FFMPEG_3_1
		AVCodecParameters *par = stream_codec(m_video_stream);

		m_codec = avcodec_find_decoder(par->codec_id);
		if (!m_codec)
		{
			//error("\"%s\" can't find decoder", file_name);
			return false;
		}

		m_codec_context = avcodec_alloc_context3(m_codec);
		if (!m_codec_context)
		{
			//error("\"%s\" can't allocate decoder context", file_name);
			return false;
		}

		int ret;

		ret = avcodec_parameters_to_context(m_codec_context, par);
		if (ret < 0)
		{
			//error("\"%s\" unsupported codec", file_name);
			return false;
		}
#else
		m_codec_context = stream_codec(m_video_stream);

		m_codec = avcodec_find_decoder(m_codec_context->codec_id);
		if (!m_codec) {
			//error("\"%s\" unsupported codec", file_name);
			return false;
		}
#endif

		if (avcodec_open2(m_codec_context, m_codec, NULL) < 0) {
			//error("\"%s\" could not open codec", file_name);
			return false;
		}
		if (!strcmp(m_codec_context->codec->name, "mjpeg") ||
			!strcmp(m_codec_context->codec->name, "dvvideo")) {
			m_offset_time = false;
		}
		m_codec_cap_delay = (bool)(m_codec_context->codec->capabilities & CODEC_CAP_DELAY);

		AVStream *stream = m_format_context->streams[m_video_stream];
		if (stream->r_frame_rate.num != 0 && stream->r_frame_rate.den != 0)
		{
			m_frame_rate = stream->r_frame_rate;
		}

		mFrameCount = m_frames = stream->nb_frames;
		m_start_time = stream->start_time;
		if (!m_frames)
		{
			Seek(0);
			AVPacket pkt;
			av_init_packet(&pkt);
			av_read_frame(m_format_context, &pkt);
			int64_t first_pts = pkt.pts;
			int64_t max_pts = 0;
			av_free_packet(&pkt); //because seek(int) uses m_format_context
			Seek(1 << 29);
			av_init_packet(&pkt); //Is this needed?
			while (stream && av_read_frame(m_format_context, &pkt) >= 0)
			{
				int64_t current_pts = static_cast<int64_t> (av_q2d(stream->time_base) * (pkt.pts - first_pts) * Fps());
				if (current_pts > max_pts)
				{
					max_pts = current_pts + 1;
				}
				av_free_packet(&pkt); //Always free before format_context usage
			}
			m_frames = max_pts;
		}
		m_frame = av_frame_alloc();
		m_rgb_frame = av_frame_alloc();

		AVPixelFormat src_pix_format;
		switch (m_codec_context->pix_fmt) { // deprecation warning for YUV formats
		case AV_PIX_FMT_YUVJ420P:
			src_pix_format = AV_PIX_FMT_YUV420P;
			break;
		case AV_PIX_FMT_YUVJ422P:
			src_pix_format = AV_PIX_FMT_YUV422P;
			break;
		case AV_PIX_FMT_YUVJ444P:
			src_pix_format = AV_PIX_FMT_YUV444P;
			break;
		case AV_PIX_FMT_YUVJ440P:
			src_pix_format = AV_PIX_FMT_YUV440P;
			break;
		default:
			src_pix_format = m_codec_context->pix_fmt;
			break;
		}

		//m_spec = ImageSpec(m_codec_context->width, m_codec_context->height, 3);

		switch (src_pix_format) {
			// support for 10-bit and 12-bit pix_fmts
		case AV_PIX_FMT_YUV420P10BE:
		case AV_PIX_FMT_YUV420P10LE:
		case AV_PIX_FMT_YUV422P10BE:
		case AV_PIX_FMT_YUV422P10LE:
		case AV_PIX_FMT_YUV444P10BE:
		case AV_PIX_FMT_YUV444P10LE:
		case AV_PIX_FMT_YUV420P12BE:
		case AV_PIX_FMT_YUV420P12LE:
		case AV_PIX_FMT_YUV422P12BE:
		case AV_PIX_FMT_YUV422P12LE:
		case AV_PIX_FMT_YUV444P12BE:
		case AV_PIX_FMT_YUV444P12LE:
			//m_spec.set_format(TypeDesc::UINT16);
			m_dst_pix_format = AV_PIX_FMT_RGB48;
			//m_stride = (size_t)(m_spec.width * 3 * 2);
			break;
		default:
			//m_spec.set_format(TypeDesc::UINT8);
			m_dst_pix_format = AV_PIX_FMT_RGB24;
			//m_stride = (size_t)(m_spec.width * 3);
			break;
		}

		m_rgb_buffer.resize(
			avpicture_get_size(m_dst_pix_format,
				m_codec_context->width,
				m_codec_context->height),
			0
		);

		m_sws_rgb_context = sws_getContext(
			m_codec_context->width,
			m_codec_context->height,
			src_pix_format,
			m_codec_context->width,
			m_codec_context->height,
			m_dst_pix_format,
			SWS_AREA,
			NULL,
			NULL,
			NULL
		);
		mWidth = m_codec_context->width;
		mHeight = m_codec_context->height;
		AVDictionaryEntry *tag = NULL;
		/*
		while ((tag = av_dict_get(m_format_context->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
		m_spec.attribute(tag->key, tag->value);
		}
		*/
		int rat[2] = { m_frame_rate.num, m_frame_rate.den };
		//m_spec.attribute("FramesPerSecond", TypeRational, &rat);
		//m_spec.attribute("oiio:Movie", true);
		//m_spec.attribute("oiio:BitsPerSample", m_codec_context->bits_per_raw_sample);
		m_nsubimages = m_frames;
		//spec = m_spec;
		return true;
	}

	bool Decoder::SeekSubimage(int subimage, int miplevel)
	{
		if (subimage < 0 || subimage >= m_nsubimages || miplevel > 0)
		{
			return false;
		}
		if (subimage == m_subimage)
		{
			return true;
		}
		m_subimage = subimage;
		m_read_frame = false;
		return true;
	}

	void *Decoder::GetRGBData()
	{
		return m_rgb_frame->data[0];
	}

	bool Decoder::Close(void)
	{
		if (m_codec_context)
			avcodec_close(m_codec_context);
		if (m_format_context)
			avformat_close_input(&m_format_context);
		av_free(m_format_context); // will free m_codec and m_codec_context
		av_frame_free(&m_frame); // free after close input
		av_frame_free(&m_rgb_frame);
		sws_freeContext(m_sws_rgb_context);
		Init();
		return true;
	}

	void Decoder::ReadFrame(int frame)
	{
		if (m_last_decoded_pos + 1 != frame)
		{
			Seek(frame);
		}
		AVPacket pkt;
		int finished = 0;
		int ret = 0;
		while ((ret = av_read_frame(m_format_context, &pkt)) == 0 || m_codec_cap_delay)
		{
			if (pkt.stream_index == m_video_stream)
			{
				if (ret < 0 && m_codec_cap_delay)
				{
					pkt.data = NULL;
					pkt.size = 0;
				}

				finished = receive_frame(m_codec_context, m_frame, &pkt);

				double pts = 0;
				if (static_cast<int64_t>(m_frame->pkt_pts) != int64_t(AV_NOPTS_VALUE))
				{
					pts = av_q2d(m_format_context->streams[m_video_stream]->time_base) *  m_frame->pkt_pts;
				}

				int current_frame = int((pts - m_start_time) * Fps() + 0.5f); //???
																			  //current_frame =   m_frame->display_picture_number;
				m_last_search_pos = current_frame;

				if (current_frame == frame && finished)
				{
					avpicture_fill
					(
						reinterpret_cast<AVPicture*>(m_rgb_frame),
						&m_rgb_buffer[0],
						m_dst_pix_format,
						m_codec_context->width,
						m_codec_context->height
					);
					sws_scale
					(
						m_sws_rgb_context,
						static_cast<uint8_t const * const *> (m_frame->data),
						m_frame->linesize,
						0,
						m_codec_context->height,
						m_rgb_frame->data,
						m_rgb_frame->linesize
					);
					m_last_decoded_pos = current_frame;
					av_free_packet(&pkt);
					break;
				}
			}
			av_free_packet(&pkt);
		}
		m_read_frame = true;
	}

#if 0
	const char *
		Decoder::metadata(const char * key)
	{
		AVDictionaryEntry * entry = av_dict_get(m_format_context->metadata, key, NULL, 0);
		return entry ? av_strdup(entry->value) : NULL;
		// FIXME -- that looks suspiciously like a memory leak
	}



	bool
		Decoder::has_metadata(const char * key)
	{
		return av_dict_get(m_format_context->metadata, key, NULL, 0); // is there a better to check exists?
	}
#endif

	bool Decoder::Seek(int frame)
	{
		int64_t offset = TimeStamp(frame);
		int flags = AVSEEK_FLAG_BACKWARD;
		avcodec_flush_buffers(m_codec_context);
		av_seek_frame(m_format_context, -1, offset, flags);
		return true;
	}

	int64_t Decoder::TimeStamp(int frame) const
	{
		int64_t timestamp = static_cast<int64_t>((static_cast<double> (frame) / (Fps() * av_q2d(m_format_context->streams[m_video_stream]->time_base))));
		if (static_cast<int64_t>(m_format_context->start_time) != int64_t(AV_NOPTS_VALUE))
		{
			timestamp += static_cast<int64_t>(static_cast<double> (m_format_context->start_time)*AV_TIME_BASE / av_q2d(m_format_context->streams[m_video_stream]->time_base));
		}
		return timestamp;
	}

	double Decoder::Fps() const
	{
		if (m_frame_rate.den)
		{
			return av_q2d(m_frame_rate);
		}
		return 1.0f;
	}

#define VIDEO_TMP_FILE "tmp.h264"
#define FINAL_FILE_NAME "record.mp4"


	using namespace std;
	void Debug(std::string str, int err) {
		Log(str.c_str());
	}

	void Encoder::Init(int width, int height, int fpsrate, int bitrate) {

		fps = fpsrate;

		int err;

		if (!(oformat = av_guess_format(NULL, VIDEO_TMP_FILE, NULL))) {
			Debug("Failed to define output format", 0);
			return;
		}

		if ((err = avformat_alloc_output_context2(&ofctx, oformat, NULL, VIDEO_TMP_FILE) < 0)) {
			Debug("Failed to allocate output context", err);
			Free();
			return;
		}

		if (!(codec = avcodec_find_encoder(oformat->video_codec))) {
			Debug("Failed to find encoder", 0);
			Free();
			return;
		}

		if (!(videoStream = avformat_new_stream(ofctx, codec))) {
			Debug("Failed to create new stream", 0);
			Free();
			return;
		}

		if (!(cctx = avcodec_alloc_context3(codec))) {
			Debug("Failed to allocate codec context", 0);
			Free();
			return;
		}

		videoStream->codecpar->codec_id = oformat->video_codec;
		videoStream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
		videoStream->codecpar->width = width;
		videoStream->codecpar->height = height;
		videoStream->codecpar->format = AV_PIX_FMT_YUV420P;
		videoStream->codecpar->bit_rate = bitrate * 1000;
		videoStream->time_base = { 1, fps };

		avcodec_parameters_to_context(cctx, videoStream->codecpar);
		cctx->time_base = { 1, fps };
		cctx->max_b_frames = 2;
		cctx->gop_size = 12;
		/*if (videoStream->codecpar->codec_id == AV_CODEC_ID_H264) {
		av_opt_set(cctx, "preset", "ultrafast", 0);
		}*/
		if (ofctx->oformat->flags & AVFMT_GLOBALHEADER) {
			cctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		}
		avcodec_parameters_from_context(videoStream->codecpar, cctx);

		if ((err = avcodec_open2(cctx, codec, NULL)) < 0) {
			Debug("Failed to open codec", err);
			Free();
			return;
		}

		if (!(oformat->flags & AVFMT_NOFILE)) {
			if ((err = avio_open(&ofctx->pb, VIDEO_TMP_FILE, AVIO_FLAG_WRITE)) < 0) {
				Debug("Failed to open file", err);
				Free();
				return;
			}
		}

		if ((err = avformat_write_header(ofctx, NULL)) < 0) {
			Debug("Failed to write header", err);
			Free();
			return;
		}

		av_dump_format(ofctx, 0, VIDEO_TMP_FILE, 1);
	}

	void Encoder::AddFrame(uint8_t *data) {
		int err;
		if (!videoFrame) {

			videoFrame = av_frame_alloc();
			videoFrame->format = AV_PIX_FMT_YUV420P;
			videoFrame->width = cctx->width;
			videoFrame->height = cctx->height;

			if ((err = av_frame_get_buffer(videoFrame, 32)) < 0) {
				Debug("Failed to allocate picture", err);
				return;
			}
		}

		if (!swsCtx) {
			swsCtx = sws_getContext(cctx->width, cctx->height, AV_PIX_FMT_RGBA, cctx->width, cctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, 0, 0, 0);
		}

		int inLinesize[1] = { 4 * cctx->width };

		// From RGB to YUV

		sws_scale(swsCtx, (const uint8_t * const *)&data, inLinesize, 0, cctx->height, videoFrame->data, videoFrame->linesize);

		videoFrame->pts = frameCounter++;

		if ((err = avcodec_send_frame(cctx, videoFrame)) < 0) {
			Debug("Failed to send frame", err);
			return;
		}

		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;

		if (avcodec_receive_packet(cctx, &pkt) == 0) {
			pkt.flags |= AV_PKT_FLAG_KEY;
			av_interleaved_write_frame(ofctx, &pkt);
			av_packet_unref(&pkt);
		}
	}

	void Encoder::Finish() {
		//DELAYED FRAMES
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;

		for (;;) {
			avcodec_send_frame(cctx, NULL);
			if (avcodec_receive_packet(cctx, &pkt) == 0) {
				av_interleaved_write_frame(ofctx, &pkt);
				av_packet_unref(&pkt);
			}
			else {
				break;
			}
		}

		av_write_trailer(ofctx);
		if (!(oformat->flags & AVFMT_NOFILE)) {
			int err = avio_close(ofctx->pb);
			if (err < 0) {
				Debug("Failed to close file", err);
			}
		}

		Free();

		Remux();
	}

	void Encoder::Free() {
		if (videoFrame) {
			av_frame_free(&videoFrame);
			videoFrame = NULL;
		}
		if (cctx) {
			avcodec_free_context(&cctx);
			cctx = NULL;
		}
		if (ofctx) {
			avformat_free_context(ofctx);
			ofctx = NULL;
		}
		if (swsCtx) {
			sws_freeContext(swsCtx);
			swsCtx = NULL;
		}
	}

	void Encoder::Remux() {
		AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
		int err;

		if ((err = avformat_open_input(&ifmt_ctx, VIDEO_TMP_FILE, 0, 0)) < 0) {
			Debug("Failed to open input file for remuxing", err);
			goto end;
		}
		if ((err = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
			Debug("Failed to retrieve input stream information", err);
			goto end;
		}
		if ((err = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, FINAL_FILE_NAME))) {
			Debug("Failed to allocate output context", err);
			goto end;
		}

		AVStream *inVideoStream = ifmt_ctx->streams[0];
		AVStream *outVideoStream = avformat_new_stream(ofmt_ctx, NULL);
		if (!outVideoStream) {
			Debug("Failed to allocate output video stream", 0);
			goto end;
		}
		outVideoStream->time_base = { 1, fps };
		avcodec_parameters_copy(outVideoStream->codecpar, inVideoStream->codecpar);
		outVideoStream->codecpar->codec_tag = 0;

		if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
			if ((err = avio_open(&ofmt_ctx->pb, FINAL_FILE_NAME, AVIO_FLAG_WRITE)) < 0) {
				Debug("Failed to open output file", err);
				goto end;
			}
		}

		if ((err = avformat_write_header(ofmt_ctx, 0)) < 0) {
			Debug("Failed to write header to output file", err);
			goto end;
		}

		AVPacket videoPkt;
		int ts = 0;
		while (true) {
			if ((err = av_read_frame(ifmt_ctx, &videoPkt)) < 0) {
				break;
			}
			videoPkt.stream_index = outVideoStream->index;
			videoPkt.pts = ts;
			videoPkt.dts = ts;
			videoPkt.duration = av_rescale_q(videoPkt.duration, inVideoStream->time_base, outVideoStream->time_base);
			ts += videoPkt.duration;
			videoPkt.pos = -1;

			if ((err = av_interleaved_write_frame(ofmt_ctx, &videoPkt)) < 0) {
				Debug("Failed to mux packet", err);
				av_packet_unref(&videoPkt);
				break;
			}
			av_packet_unref(&videoPkt);
		}

		av_write_trailer(ofmt_ctx);

	end:
		if (ifmt_ctx) {
			avformat_close_input(&ifmt_ctx);
		}
		if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
			avio_closep(&ofmt_ctx->pb);
		}
		if (ofmt_ctx) {
			avformat_free_context(ofmt_ctx);
		}
	}
}
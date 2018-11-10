#include "ffmpegDecode.h"

#include <iostream>

namespace FFMPEG
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



	void FFmpegDecoder::RegisterAll()
	{
		av_register_all();
	}

	bool FFmpegDecoder::Open(const std::string &name)
	{
		// Temporary workaround: refuse to open a file whose name does not
		// indicate that it's a movie file. This avoids the problem that ffmpeg
		// is willing to open tiff and other files better handled by other
		// plugins. The better long-term solution is to replace av_register_all
		// with our own function that registers only the formats that we want
		// this reader to handle. At some point, we will institute that superior
		// approach, but in the mean time, this is a quick solution that 90%
		// does the job.

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

	bool FFmpegDecoder::SeekSubimage(int subimage, int miplevel)
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

	void *FFmpegDecoder::GetRGBData()
	{
		return m_rgb_frame->data[0];
	}

	bool FFmpegDecoder::Close(void)
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

	void FFmpegDecoder::ReadFrame(int frame)
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
		FFmpegDecoder::metadata(const char * key)
	{
		AVDictionaryEntry * entry = av_dict_get(m_format_context->metadata, key, NULL, 0);
		return entry ? av_strdup(entry->value) : NULL;
		// FIXME -- that looks suspiciously like a memory leak
	}



	bool
		FFmpegDecoder::has_metadata(const char * key)
	{
		return av_dict_get(m_format_context->metadata, key, NULL, 0); // is there a better to check exists?
	}
#endif

	bool FFmpegDecoder::Seek(int frame)
	{
		int64_t offset = TimeStamp(frame);
		int flags = AVSEEK_FLAG_BACKWARD;
		avcodec_flush_buffers(m_codec_context);
		av_seek_frame(m_format_context, -1, offset, flags);
		return true;
	}

	int64_t FFmpegDecoder::TimeStamp(int frame) const
	{
		int64_t timestamp = static_cast<int64_t>((static_cast<double> (frame) / (Fps() * av_q2d(m_format_context->streams[m_video_stream]->time_base))));
		if (static_cast<int64_t>(m_format_context->start_time) != int64_t(AV_NOPTS_VALUE))
		{
			timestamp += static_cast<int64_t>(static_cast<double> (m_format_context->start_time)*AV_TIME_BASE / av_q2d(m_format_context->streams[m_video_stream]->time_base));
		}
		return timestamp;
	}

	double FFmpegDecoder::Fps() const
	{
		if (m_frame_rate.den)
		{
			return av_q2d(m_frame_rate);
		}
		return 1.0f;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	const char *err2str(int errnum)
	{
		static char errStr[AV_ERROR_MAX_STRING_SIZE];
		av_make_error_string(errStr, AV_ERROR_MAX_STRING_SIZE, errnum);
		return errStr;
	}

	bool ofxFFMPEGVideoWriter::setup(const char* filename, int width, int height, int bitrate, int framerate) 
	{
		if (initialized)
		{
			Log("Stream already initialized. Close it before.");
			return false;
		}

		Log("Video encoding: %s\n", filename);
		/* register all the formats and codecs */

		/* allocate the output media context */
		avformat_alloc_output_context2(&oc, NULL, NULL, filename);
		if (!oc) {
			Log("Could not deduce output format from file extension: using MPEG.\n");
			avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
		}
		if (!oc) {
			Log("could not create AVFormat context\n");
			return false;
		}
		fmt = oc->oformat;

		AVPixelFormat supported_pix_fmt = AV_PIX_FMT_NONE;

		/* Add the audio and video streams using the default format codecs
		* and initialize the codecs. */
		video_st = NULL;
		if (fmt->video_codec != AV_CODEC_ID_NONE) {
			/* find the video encoder */
			AVCodecID avcid = fmt->video_codec;
			codec = avcodec_find_encoder(avcid);
			if (!codec) {
				Log("codec not found: %s\n", avcodec_get_name(avcid));
				return false;
			}
			else {
				const AVPixelFormat* p = codec->pix_fmts;
				while (p != NULL && *p != AV_PIX_FMT_NONE) {
					Log("supported pix fmt: %s\n", av_get_pix_fmt_name(*p));
					supported_pix_fmt = *p;
					++p;
				}
				if (p == NULL || *p == AV_PIX_FMT_NONE) {
					if (fmt->video_codec == AV_CODEC_ID_RAWVIDEO) {
						supported_pix_fmt = AV_PIX_FMT_RGB24;
					}
					else {
						supported_pix_fmt = AV_PIX_FMT_YUV420P; /* default pix_fmt */
					}
				}
			}

			video_st = avformat_new_stream(oc, codec);
			if (!video_st) {
				Log("Could not allocate stream\n");
				return false;
			}
			video_st->id = oc->nb_streams - 1;
			c = video_st->codec;

			/* Some formats want stream headers to be separate. */
			
			if (oc->oformat->flags & AVFMT_GLOBALHEADER)
				c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
				
		}

		/* Now that all the parameters are set, we can open the audio and
		* video codecs and allocate the necessary encode buffers. */
		{

			picture = av_frame_alloc();
			picture->pts = 0;

			/* put sample parameters */
			c->codec_id = fmt->video_codec;
			c->codec_type = AVMEDIA_TYPE_VIDEO;
			c->bit_rate = bitrate;
			// resolution must be a multiple of two
			c->width = width;
			c->height = height;
			// frames per second
			c->time_base = /*(AVRational)*/{ 1, framerate };
			c->pix_fmt = supported_pix_fmt;
			//        c->gop_size = 10; // emit one intra frame every ten frames
			//        c->max_b_frames=1;
			//
			//        { //try to get the default pix format
			//            AVCodecContext tmpcc;
			//            avcodec_get_context_defaults3(&tmpcc, c->codec);
			//            c->pix_fmt = (tmpcc.pix_fmt != AV_PIX_FMT_NONE) ? tmpcc.pix_fmt : AV_PIX_FMT_YUV420P;
			//        }
			int ret = 0;

			/* open it */
			//        if(c->codec_id != AV_CODEC_ID_RAWVIDEO) {
			AVDictionary* options = NULL;
			if ((ret = avcodec_open2(c, codec, &options)) < 0) {
				Log("Could not open codec: %s\n", err2str(ret));
				return false;
			}
			else {
				Log("opened %s\n", avcodec_get_name(fmt->video_codec));
			}
			//        } else
			//            Log("raw video, no codec\n");

			/* alloc image and output buffer */
			picture->data[0] = NULL;
			picture->linesize[0] = -1;
			picture->format = c->pix_fmt;

			ret = av_image_alloc(picture->data, picture->linesize, c->width, c->height, (AVPixelFormat)picture->format, 32);
			if (ret < 0) {
				Log("Could not allocate raw picture buffer: %s\n", err2str(ret));
				return false;
			}
			else {
				Log("allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", ret, picture->data[0], picture->linesize[0], picture->linesize[1], picture->linesize[2], picture->linesize[3]);
			}

			picture_rgb24 = av_frame_alloc();
			picture_rgb24->format = AV_PIX_FMT_RGB24;

			if ((ret = av_image_alloc(picture_rgb24->data, picture_rgb24->linesize, c->width, c->height, (AVPixelFormat)picture_rgb24->format, 24)) < 0) {
				Log("cannot allocate RGB temp image\n");
				return false;
			}
			else
				Log("allocated picture of size %d (ptr %x), linesize %d %d %d %d\n", ret, picture_rgb24->data[0], picture_rgb24->linesize[0], picture_rgb24->linesize[1], picture_rgb24->linesize[2], picture_rgb24->linesize[3]);


			size = ret;
		}

		av_dump_format(oc, 0, filename, 1);
		/* open the output file, if needed */
		if (!(fmt->flags & AVFMT_NOFILE)) {
			int ret;
			if ((ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE)) < 0) {
				Log("Could not open '%s': %s\n", filename, err2str(ret));
				return false;
			}
		}
		/* Write the stream header, if any. */
		int ret = avformat_write_header(oc, NULL);
		if (ret < 0) {
			Log("Error occurred when opening output file: %s\n", err2str(ret));
			return false;
		}

		/* get sws context for RGB24 -> YUV420 conversion */
		sws_ctx = sws_getContext(c->width, c->height, (AVPixelFormat)picture_rgb24->format,
			c->width, c->height, (AVPixelFormat)picture->format,
			SWS_BICUBIC, NULL, NULL, NULL);
		if (!sws_ctx) {
			Log("Could not initialize the conversion context\n");
			return false;
		}

		initialized = true;
		return true;
	}

	/* add a frame to the video file, RGB 24bpp format */
	bool ofxFFMPEGVideoWriter::addFrame(const uint8_t* pixels) 
	{
		if (!initialized)
		{
			Log("Trying to add frame on an uninitialized video stream.");
			return false;
		}
		/* copy the buffer */
		memcpy(picture_rgb24->data[0], pixels, size);

		/* convert RGB24 to YUV420 */
		sws_scale(sws_ctx, picture_rgb24->data, picture_rgb24->linesize, 0, c->height, picture->data, picture->linesize);

		int ret = -1;
#if 0
		if (oc->oformat->flags & AVFMT_RAWPICTURE) {
			/* Raw video case - directly store the picture in the packet */
			AVPacket pkt;
			av_init_packet(&pkt);
			pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.stream_index = video_st->index;
			pkt.data = picture->data[0];
			pkt.size = sizeof(AVPicture);
			ret = av_interleaved_write_frame(oc, &pkt);
		}
		else
#endif
		{
			AVPacket pkt = { 0 };
			int got_packet;
			av_init_packet(&pkt);
			/* encode the image */
			ret = avcodec_encode_video2(c, &pkt, picture, &got_packet);
			if (ret < 0)
			{
				Log("Error encoding video frame: %s\n", err2str(ret));
				return false;
			}
			/* If size is zero, it means the image was buffered. */
			if (!ret && got_packet && pkt.size) {
				pkt.stream_index = video_st->index;
				/* Write the compressed frame to the media file. */
				ret = av_interleaved_write_frame(oc, &pkt);
			}
			else {
				ret = 0;
			}
		}
		picture->pts += av_rescale_q(1, video_st->codec->time_base, video_st->time_base);
		frame_count++;
		return true;
	}


	void ofxFFMPEGVideoWriter::close() 
	{
		if (!initialized)
			return;
		/* Write the trailer, if any. The trailer must be written before you
		* close the CodecContexts open when you wrote the header; otherwise
		* av_write_trailer() may try to use memory that was freed on
		* av_codec_close(). */
		av_write_trailer(oc);
		/* Close each codec. */

		avcodec_close(video_st->codec);
		av_freep(&(picture->data[0]));
		av_free(picture);

		if (!(fmt->flags & AVFMT_NOFILE))
			/* Close the output file. */
			avio_close(oc->pb);

		/* free the stream */
		avformat_free_context(oc);

		Log("closed video file\n");

		initialized = false;
		frame_count = 0;
	}


}
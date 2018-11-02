#pragma once

#include "ffmpegInclude.h"
#include <string>
#include <vector>

class FFmpegDecoder
{ 
public:
	bool open(const std::string &name);
	bool close(void);
	int current_subimage(void) const {
		return m_subimage;
	}
	bool seek_subimage(int subimage, int miplevel);

	void *getRGBData();
	void read_frame(int pos);
#if 0
	const char *metadata(const char * key);
	bool has_metadata(const char * key);
#endif
	bool seek(int pos);
	double fps() const;
	int64_t time_stamp(int pos) const;

	size_t mWidth, mHeight;
	size_t mFrameCount;

private:

	std::string m_filename;
	int m_subimage;
	int64_t m_nsubimages;
	AVFormatContext * m_format_context;
	AVCodecContext * m_codec_context;
	AVCodec *m_codec;
	AVFrame *m_frame;
	AVFrame *m_rgb_frame;
	size_t m_stride;
	AVPixelFormat m_dst_pix_format;
	SwsContext *m_sws_rgb_context;
	AVRational m_frame_rate;
	std::vector<uint8_t> m_rgb_buffer;
	std::vector<int> m_video_indexes;
	int m_video_stream;
	int64_t m_frames;
	int m_last_search_pos;
	int m_last_decoded_pos;
	bool m_offset_time;
	bool m_codec_cap_delay;
	bool m_read_frame;
	int64_t m_start_time;

	// init to initialize state
	void init(void) {
		m_filename.clear();
		m_format_context = 0;
		m_codec_context = 0;
		m_codec = 0;
		m_frame = 0;
		m_rgb_frame = 0;
		m_sws_rgb_context = 0;
		m_stride = 0;
		m_rgb_buffer.clear();
		m_video_indexes.clear();
		m_video_stream = -1;
		m_frames = 0;
		m_last_search_pos = 0;
		m_last_decoded_pos = 0;
		m_offset_time = true;
		m_read_frame = false;
		m_codec_cap_delay = false;
		m_subimage = 0;
		m_start_time = 0;
	}
};

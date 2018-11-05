#pragma once

#include "ffmpegInclude.h"
#include <string>
#include <vector>

namespace FFMPEG
{

	using namespace std;

class FFmpegDecoder
{ 
public:
	static void RegisterAll();
	FFmpegDecoder() { Init(); }
	bool Open(const std::string &name);
	bool Close(void);
	int CurrentSubimage(void) const {
		return m_subimage;
	}
	bool SeekSubimage(int subimage, int miplevel);

	void *GetRGBData();
	void ReadFrame(int pos);
#if 0
	const char *metadata(const char * key);
	bool has_metadata(const char * key);
#endif
	bool Seek(int pos);
	double Fps() const;
	int64_t TimeStamp(int pos) const;

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
	void Init(void) {
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
class ofxFFMPEGVideoWriter {
	//instance variables
	AVCodec *codec;
	int size, frame_count;
	AVFrame *picture, *picture_rgb24;
	struct SwsContext *sws_ctx;
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVStream *video_st;
	AVCodecContext* c;

	bool initialized;

public:

	ofxFFMPEGVideoWriter() :oc(NULL), codec(NULL), initialized(false), frame_count(1) {}
	~ofxFFMPEGVideoWriter() { close(); }
	/**
	* setup the video writer
	* @param output filename, the codec and format will be determined by it. (e.g. "xxx.mpg" will create an MPEG1 file
	* @param width of the frame
	* @param height of the frame
	* @param the bitrate
	* @param the framerate
	**/
	bool setup(const char* filename, int width, int height, int bitrate = 400000, int framerate = 25);
	/**
	* add a frame to the video file
	* @param the pixels packed in RGB (24-bit RGBRGBRGB...)
	**/
	bool addFrame(const uint8_t* pixels);
	/**
	* close the video file and release all datastructs
	**/
	void close();
	/**
	* is the videowriter initialized?
	**/
	bool isInitialized() const { return initialized; }
};

extern int(*Log)(const char *szFormat, ...);
}
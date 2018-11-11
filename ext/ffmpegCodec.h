#pragma once

#include "ffmpegInclude.h"
#include <string>
#include <vector>





#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <math.h>
#include <string.h>
#include <algorithm>
#include <string> 

namespace FFMPEGCodec
{
	void RegisterAll();

	using namespace std;

	class Decoder
	{
	public:
		Decoder() { Init(); }
		virtual ~Decoder() 
		{ 
			Close(); 
		}
		bool Open(const std::string &name);
		bool Close(void);
		int CurrentSubimage(void) const {
			return m_subimage;
		}
		bool SeekSubimage(int subimage, int miplevel);

		void *GetRGBData();
		void ReadFrame(int pos);

		bool Seek(int pos);
		double Fps() const;
		int64_t TimeStamp(int pos) const;

		size_t mWidth, mHeight;
		size_t mFrameCount;
		const std::string GetFilename() const { return m_filename; }
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
			mFrameCount = 0;
		}
	};
	
	
	class Encoder {
	public:

		Encoder() {
			oformat = NULL;
			ofctx = NULL;
			videoStream = NULL;
			videoFrame = NULL;
			swsCtx = NULL;
			frameCounter = 0;

			// Initialize libavcodec
			//av_register_all();
			//av_log_set_callback(avlog_cb);
		}

		~Encoder() {
			Free();
		}

		void Init(int width, int height, int fpsrate, int bitrate);

		void AddFrame(uint8_t *data);

		void Finish();

	private:

		AVOutputFormat *oformat;
		AVFormatContext *ofctx;

		AVStream *videoStream;
		AVFrame *videoFrame;

		AVCodec *codec;
		AVCodecContext *cctx;

		SwsContext *swsCtx;

		int frameCounter;

		int fps;

		void Free();

		void Remux();
	};
	/*
	Encoder* Init(int width, int height, int fps, int bitrate) {
		Encoder *vc = new Encoder();
		vc->Init(width, height, fps, bitrate);
		return vc;
	};

	void AddFrame(uint8_t *data, Encoder *vc) {
		vc->AddFrame(data);
	}

	void Finish(Encoder *vc) {
		vc->Finish();
	}
	*/
	/*
	void SetDebug(FuncPtr fp) {
	   ExtDebug = fp;
   };
   */


	extern int(*Log)(const char *szFormat, ...);
}
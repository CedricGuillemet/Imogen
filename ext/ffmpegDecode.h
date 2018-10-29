/*
	It is FFmpeg decoder class. Sample for article from unick-soft.ru
*/

#ifndef __FFMPEG_DECODER__
#define __FFMPEG_DECODER__

#include "ffmpegInclude.h"
#include <string>

class FFmpegDecoder
{ 
  // constructor.
  public: 
	  FFmpegDecoder() : pImgConvertCtx(NULL), audioBaseTime(0.0), videoBaseTime(0.0),
          videoFramePerSecond(0.0), isOpen(false), audioStreamIndex(-1), videoStreamIndex(-1),
          pAudioCodec(NULL), pAudioCodecCtx(NULL), pVideoCodec(NULL), pVideoCodecCtx(NULL),
          pFormatCtx(NULL), videoDuration(0.f) {}

		  static void RegisterAll();
  // destructor.
  ~FFmpegDecoder() 
  {
    CloseFile();
  }
  /*
  FFmpegDecoder* operator = (const FFmpegDecoder& other)
  {

	  return this;
  }
  */
  
  bool OpenFile(std::string& inputFile);

  // Close file and free resourses.
  bool CloseFile();

  // Return next frame FFmpeg.
  AVFrame * GetNextFrame();

		  float GetDuration() const { return videoDuration;  }
		  size_t GetFrameDuration() const { return videoFrameDuration; }
  int GetWidth()
  {
    return width;
  }
  int GetHeight()
  {
    return height;
  }

  bool Seek(size_t frame);
  // open video stream.
  bool OpenVideo();

  // open audio stream.
  bool OpenAudio();

  // close video stream.
  void CloseVideo();

  // close audio stream.
  void CloseAudio();

private:
  AVFrame * GetRGBAFrame(AVFrame *pFrameYuv);
  private: int DecodeAudio(int nStreamIndex, const AVPacket *avpkt, 
                                    uint8_t* pOutBuffer, size_t nOutBufferSize);
  bool DecodeVideo(const AVPacket *avpkt, AVFrame * pOutFrame);
  
  AVFormatContext* pFormatCtx;  
  AVCodecContext* pVideoCodecCtx;
  AVCodec* pVideoCodec;
  AVCodecContext* pAudioCodecCtx;
  AVCodec* pAudioCodec;
  int videoStreamIndex;
  int audioStreamIndex;
  bool isOpen;
  double videoFramePerSecond;
  double videoBaseTime;
  double audioBaseTime;
  struct SwsContext *pImgConvertCtx;
  int width;
  int height;
  float videoDuration;
  size_t videoFrameDuration;
};

#endif
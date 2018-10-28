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
  public: FFmpegDecoder() : pImgConvertCtx(NULL), audioBaseTime(0.0), videoBaseTime(0.0),
          videoFramePerSecond(0.0), isOpen(false), audioStreamIndex(-1), videoStreamIndex(-1),
          pAudioCodec(NULL), pAudioCodecCtx(NULL), pVideoCodec(NULL), pVideoCodecCtx(NULL),
          pFormatCtx(NULL), videoDuration(0.f) {}

  // destructor.
  public: virtual ~FFmpegDecoder() 
  {
    CloseFile();
  }

  // Open file
  public: virtual bool OpenFile(std::string& inputFile);

  // Close file and free resourses.
  public: virtual bool CloseFile();

  // Return next frame FFmpeg.
  public: virtual AVFrame * GetNextFrame();

		  float GetDuration() const { return videoDuration;  }
		  size_t GetFrameDuration() const { return videoFrameDuration; }
  public: int GetWidth()
  {
    return width;
  }
  public: int GetHeight()
  {
    return height;
  }

  // open video stream.
  private: bool OpenVideo();

  // open audio stream.
  private: bool OpenAudio();

  // close video stream.
  private: void CloseVideo();

  // close audio stream.
  private: void CloseAudio();

  // return rgb image 
  private: AVFrame * GetRGBAFrame(AVFrame *pFrameYuv);

  // Decode audio from packet.
  private: int DecodeAudio(int nStreamIndex, const AVPacket *avpkt, 
                                    uint8_t* pOutBuffer, size_t nOutBufferSize);

  // Decode video buffer.
  private: bool DecodeVideo(const AVPacket *avpkt, AVFrame * pOutFrame);

  // FFmpeg file format.
  private: AVFormatContext* pFormatCtx;  

  // FFmpeg codec context.
  private: AVCodecContext* pVideoCodecCtx;

  // FFmpeg codec for video.
  private: AVCodec* pVideoCodec;

  // FFmpeg codec context for audio.
  private: AVCodecContext* pAudioCodecCtx;

  // FFmpeg codec for audio.
  private: AVCodec* pAudioCodec;

  // Video stream number in file.
  private: int videoStreamIndex;

  // Audio stream number in file.
  private: int audioStreamIndex;

  // File is open or not.
  private: bool isOpen;

  // Video frame per seconds.
  private: double videoFramePerSecond;

  // FFmpeg timebase for video.
  private: double videoBaseTime;
 
  // FFmpeg timebase for audio.
  private: double audioBaseTime;

  // FFmpeg context convert image.
  private: struct SwsContext *pImgConvertCtx;

  // Width of image
  private: int width;
  
  // Height of image
  private: int height;

		   float videoDuration;
		   size_t videoFrameDuration;
};

#endif
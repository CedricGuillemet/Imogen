/*
	It is FFmpeg decoder class. Sample for article from unick-soft.ru
*/

#include "ffmpegDecode.h"

#define min(a,b) (a > b ? b : a)

#define MAX_AUDIO_PACKET (2 * 1024 * 1024)

void FFmpegDecoder::RegisterAll()
{
	// Register all components
	av_register_all();
}

bool FFmpegDecoder::OpenFile (std::string& inputFile)
{
	CloseFile();


	// Open media file.
	if (avformat_open_input(&pFormatCtx, inputFile.c_str(), NULL, NULL) != 0)
	{
		CloseFile();
		return false;
	}

	// Get format info.
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		CloseFile();
		return false;
	}

	// open video and audio stream.
	bool hasVideo = OpenVideo();
	bool hasAudio = OpenAudio(); 

	if (!hasVideo)
	{
		CloseFile();
		return false;
	}

	isOpen = true;

	// Get file information.
	if (videoStreamIndex != -1)
	{
		videoFramePerSecond = av_q2d(pFormatCtx->streams[videoStreamIndex]->r_frame_rate);
		// Need for convert time to ffmpeg time.
		videoBaseTime       = av_q2d(pFormatCtx->streams[videoStreamIndex]->time_base); 
		videoDuration		= (float)((double)pFormatCtx->streams[videoStreamIndex]->duration * videoBaseTime); 
		videoFrameDuration = size_t(videoDuration * videoFramePerSecond);
	}

	if (audioStreamIndex != -1)
	{
		audioBaseTime = av_q2d(pFormatCtx->streams[audioStreamIndex]->time_base);
	}

	return true;
}


bool FFmpegDecoder::CloseFile()
{
	isOpen = false;

	// Close video and audio.
	CloseVideo();
	CloseAudio();

	if (pFormatCtx)
	{
		avformat_close_input(&pFormatCtx);
		pFormatCtx = NULL;
	}

	return true;
}


AVFrame * FFmpegDecoder::GetNextFrame()
{
	AVFrame * res = NULL;

	if (videoStreamIndex != -1)
	{
		AVFrame *pVideoYuv = av_frame_alloc();
		AVPacket packet;

		if (isOpen)
		{
			// Read packet.
			while (av_read_frame(pFormatCtx, &packet) >= 0)
			{
				int64_t pts = 0;
				pts = (packet.dts != AV_NOPTS_VALUE) ? packet.dts : 0;

				if(packet.stream_index == videoStreamIndex)
				{
					// Convert ffmpeg frame timestamp to real frame number.
					int64_t numberFrame = (double)((int64_t)pts - 
						pFormatCtx->streams[videoStreamIndex]->start_time) * 
						videoBaseTime * videoFramePerSecond; 

					// Decode frame
					bool isDecodeComplite = DecodeVideo(&packet, pVideoYuv);
					if (isDecodeComplite)
					{
						res = GetRGBAFrame(pVideoYuv);
					}
					break;
				} 
				else if (packet.stream_index == audioStreamIndex)
				{
					if (packet.dts != AV_NOPTS_VALUE)
					{
						int audioFrameSize = MAX_AUDIO_PACKET;
						uint8_t * pFrameAudio = new uint8_t[audioFrameSize];
						if (pFrameAudio)
						{
							double fCurrentTime = (double)(pts - pFormatCtx->streams[videoStreamIndex]->start_time)
								* audioBaseTime;
							double fCurrentDuration = (double)packet.duration * audioBaseTime;

							// Decode audio
							int nDecodedSize = DecodeAudio(audioStreamIndex, &packet,
								pFrameAudio, audioFrameSize);

							if (nDecodedSize > 0 && pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
							{
								// Process audio here.
								/* Uncommend sample if you want write raw data to file.
								{
									int size = nDecodedSize / sizeof(float);
									signed short * ar = new signed short[nDecodedSize / sizeof(float)];
									float* pointer = (float*)pFrameAudio;
									// Convert float to S16.
									for (int i = 0; i < size / 2; i ++)
									{
										ar[i] = pointer[i] * 32767.0f;
									}

									FILE* file = fopen("c:\\temp\\AudioRaw.raw", "ab");
									fwrite(ar, 1, size * sizeof (signed short) / 2, file);
									fclose(file);
								}
								*/
							}
							
							// Delete buffer.
							delete[] pFrameAudio;
							pFrameAudio = NULL;
						}
					}
				}

				av_free_packet(&packet);
				packet = AVPacket();
			}

			av_free(pVideoYuv);
		}    
	}

	return res;
}


AVFrame * FFmpegDecoder::GetRGBAFrame(AVFrame *pFrameYuv)
{
	AVFrame * frame = NULL;
	int width  = pVideoCodecCtx->width;
	int height = pVideoCodecCtx->height;
	int bufferImgSize = avpicture_get_size(AV_PIX_FMT_BGR24, width, height);
	frame = av_frame_alloc();
	uint8_t * buffer = (uint8_t*)av_mallocz(bufferImgSize);
	if (frame)
	{
		avpicture_fill((AVPicture*)frame, buffer, AV_PIX_FMT_BGR24, width, height);
		frame->width  = width;
		frame->height = height;
		//frame->data[0] = buffer;

		sws_scale(pImgConvertCtx, pFrameYuv->data, pFrameYuv->linesize,
			0, height, frame->data, frame->linesize);      
	}  

	return (AVFrame *)frame;
}


bool FFmpegDecoder::OpenVideo()
{
	bool res = false;

	if (pFormatCtx)
	{
		videoStreamIndex = -1;

		for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
		{
			if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				videoStreamIndex = i;
				pVideoCodecCtx = pFormatCtx->streams[i]->codec;
				pVideoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);

				if (pVideoCodec)
				{
					res     = !(avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0);
					width   = pVideoCodecCtx->coded_width;
					height  = pVideoCodecCtx->coded_height;
				}

				break;
			}
		}

		if (!res)
		{
			CloseVideo();
		}
		else
		{
			pImgConvertCtx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height,
				pVideoCodecCtx->pix_fmt,
				pVideoCodecCtx->width, pVideoCodecCtx->height,
				AV_PIX_FMT_BGR24,
				SWS_BICUBIC, NULL, NULL, NULL);
		}
	}

	return res;
}

bool FFmpegDecoder::DecodeVideo(const AVPacket *avpkt, AVFrame * pOutFrame)
{
	bool res = false;

	if (pVideoCodecCtx)
	{
		if (avpkt && pOutFrame)
		{
			int got_picture_ptr = 0;
			int videoFrameBytes = avcodec_decode_video2(pVideoCodecCtx, pOutFrame, &got_picture_ptr, avpkt);

//			avcodec_decode_video(pVideoCodecCtx, pOutFrame, &videoFrameBytes, pInBuffer, nInbufferSize);
			res = (videoFrameBytes > 0);
		}
	}

	return res;
}


bool FFmpegDecoder::OpenAudio()
{
	bool res = false;

	if (pFormatCtx)
	{   
		audioStreamIndex = -1;

		for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
		{
			if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				audioStreamIndex = i;
				pAudioCodecCtx = pFormatCtx->streams[i]->codec;
				pAudioCodec = avcodec_find_decoder(pAudioCodecCtx->codec_id);
				if (pAudioCodec)
				{
					res = !(avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0);       
				}
				break;
			}
		}

		if (! res)
		{
			CloseAudio();
		}
	}

	return res;
}


void FFmpegDecoder::CloseVideo()
{
	if (pVideoCodecCtx)
	{
		avcodec_close(pVideoCodecCtx);
		pVideoCodecCtx = NULL;
		pVideoCodec = NULL;
		videoStreamIndex = 0;
	}
}


void FFmpegDecoder::CloseAudio()
{    
	if (pAudioCodecCtx)
	{
		avcodec_close(pAudioCodecCtx);
		pAudioCodecCtx   = NULL;
		pAudioCodec      = NULL;
		audioStreamIndex = 0;
	}  
}


int FFmpegDecoder::DecodeAudio(int nStreamIndex, const AVPacket *avpkt, uint8_t* pOutBuffer, size_t nOutBufferSize)
{
	int decodedSize = 0;

	int packetSize = avpkt->size;
	uint8_t* pPacketData = (uint8_t*) avpkt->data;

	while (packetSize > 0)
	{
		int sizeToDecode = nOutBufferSize;
		uint8_t* pDest = pOutBuffer + decodedSize;
		int got_picture_ptr = 0;
		AVFrame* audioFrame = av_frame_alloc();

		int packetDecodedSize = avcodec_decode_audio4(pAudioCodecCtx, audioFrame, &got_picture_ptr, avpkt);

		if (packetDecodedSize > 0)
		{
			sizeToDecode = av_samples_get_buffer_size(NULL, audioFrame->channels,
                                                       audioFrame->nb_samples,
													   (AVSampleFormat)audioFrame->format, 1);

			// Currenlty we process only AV_SAMPLE_FMT_FLTP.
			if ((AVSampleFormat)audioFrame->format == AV_SAMPLE_FMT_FLTP)
			{
				// Copy each channel plane.
				for (int i = 0; i < audioFrame->channels; i++)
				{
					memcpy(pDest + i * sizeToDecode / audioFrame->channels, 
						audioFrame->extended_data[i], sizeToDecode / audioFrame->channels);
				}
			}
		}

		if (packetDecodedSize < 0)
		{
			decodedSize = 0;
			break;
		}

		packetSize -= packetDecodedSize;
		pPacketData += packetDecodedSize;

		if (sizeToDecode <= 0)
		{
			continue;
		}

		decodedSize += sizeToDecode;
		av_frame_free(&audioFrame);
		audioFrame = NULL;
	}	

	return decodedSize;
}

bool FFmpegDecoder::Seek(size_t frame)
{
	double m_out_start_time = frame / videoFramePerSecond;
	int flgs = AVSEEK_FLAG_ANY;

	auto m_in_vid_strm = pFormatCtx->streams[videoStreamIndex];

	int seek_ts = (m_out_start_time*(m_in_vid_strm->time_base.den)) / (m_in_vid_strm->time_base.num);
	if (av_seek_frame(pFormatCtx, videoStreamIndex, frame, flgs) < 0)
	{
		return false;
	}
	return true;
}
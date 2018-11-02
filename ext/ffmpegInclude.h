/*
 *  Include ffmpeg files
 */


#pragma once

#ifndef __STDC_CONSTANT_MACROS
  #define __STDC_CONSTANT_MACROS
#endif

//#include "stdint.h"
extern "C" 
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(57,24,0)
# include <libavutil/imgutils.h>
#endif
}
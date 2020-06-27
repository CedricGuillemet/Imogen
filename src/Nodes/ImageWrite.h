// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
//
// Copyright(c) 2019 Cedric Guillemet
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

struct ImageWriteBlock
{
    char filename[1024];
    int format;
    int quality;
    int width;
    int height;
    int mode;
};

DECLARE_NODE(ImageWrite)
{
    ImageWriteBlock* param = (ImageWriteBlock*)parameters;
    const char* stockImages[] = {"Stock/jpg-icon.png", "Stock/png-icon.png", "Stock/tga-icon.png", "Stock/bmp-icon.png", "Stock/hdr-icon.png", "Stock/dds-icon.png", "Stock/ktx-icon.png", "Stock/mp4-icon.png"};
    Image image;
	const NodeIndex source = int(evaluation->inputIndices[0]);
    param->width = std::max(param->width, 8);
    param->height = std::max(param->height, 8);
	NodeIndex target = int(evaluation->targetIndex);

    // set info stock image
    if (Image::Read(stockImages[param->format], &image) == EVAL_OK)
    {
        if (SetEvaluationImage(context, target, &image) == EVAL_OK)
        {
        }
    }

    if (!source.IsValid())
    {
        return EVAL_OK;
    }
    int imageWidth, imageHeight;
    int res = GetEvaluationSize(context, source, &imageWidth, &imageHeight);
    if (param->mode && res == EVAL_OK)
    {
        const float ratio = float(imageWidth) / float(imageHeight);
        if (param->mode == 1)
        {
            param->height = int(param->width / ratio);
        }
        else if (param->mode == 2)
        {
            param->width = int(param->height * ratio);
        }
		else if (param->mode == 3)
		{
			param->width = imageWidth;
			param->height = imageHeight;
		}
    }
    
	if (!IsBuilding(context))
    {
        return EVAL_OK;
    }
    
	
	if (GetEvaluationImage(context, source, &image) == EVAL_OK)
	{
		auto sampler = context->mEvaluationStages.mInputSamplers[target][0];
		Image::Resize(&image, param->width, param->height, sampler);
        if (Image::Write(param->filename, &image, param->format, param->quality) == EVAL_OK)
        {    
            Log("Image %s saved.\n", param->filename);
            return EVAL_OK;
        }
        else
        {
            Log("Unable to write image : %s \n", param->filename);
        }
    }

    return EVAL_ERR;
}
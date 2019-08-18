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

struct ImageReadBlock
{
    char filename[1024];
    char XPosFilename[1024];
    char XNegFilename[1024];
    char YPosFilename[1024];
    char YNegFilename[1024];
    char ZPosFilename[1024];
    char ZNegFilename[1024];
};

DECLARE_NODE(ImageRead)
{
    ImageReadBlock* params = (ImageReadBlock*)parameters;

    const char *files[] = { params->XPosFilename, params->XNegFilename, params->YNegFilename, params->YPosFilename, params->ZPosFilename, params->ZNegFilename};
    if (!(evaluation->dirtyFlag & Dirty::Parameter))
    {
        return EVAL_OK;
    }

	int target = int(evaluation->targetIndex);
	SetEvaluationPersistent(context, target, 1);

    if (strlen(params->filename))
    {
        SetProcessing(context, target, 1);
        ReadImageAsync(context, params->filename, evaluation->targetIndex, -1);
    }
    else
    {
        for (int i = 0; i < 6; i++)
        {
            if (!strlen(files[i]))
            {
                return EVAL_OK;
            }
        }
        SetProcessing(context, target, 1);
        for (int i = 0; i < 6; i++)
        {
            ReadImageAsync(context, files[i], target, i);
        }        
    }

    return EVAL_OK;
}
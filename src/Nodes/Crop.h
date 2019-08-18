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

struct CropBlock
{
    float quad[4];
};

DECLARE_NODE(Crop)
{
    CropBlock* params = (CropBlock*)parameters;
    int croppedWidth = 256;
    int croppedHeight = 256;
    int width, height;
    const int res = GetEvaluationSize(context, int(evaluation->inputIndices[0]), &width, &height);
    if (res == EVAL_OK)
    {
        croppedWidth = width * int(fabsf(params->quad[2] - params->quad[0]));
        croppedHeight = height * int(fabsf(params->quad[3] - params->quad[1]));
    }
    //if (croppedWidth<8) { croppedWidth = 8; }
    //if (croppedHeight<8) { croppedHeight = 8; }
	int target = int(evaluation->targetIndex);
	SetEvaluationPersistent(context, target, 1);
    if (evaluation->uiPass)
    {
        SetEvaluationSize(context, target, width, height);
    }
    else
    {
        SetEvaluationSize(context, target, croppedWidth, croppedHeight);
    }
    
    return EVAL_OK;
}


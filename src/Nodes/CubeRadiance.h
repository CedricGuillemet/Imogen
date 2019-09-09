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

struct CubeRadianceBlock
{
    int mode;
    int size;
    int sampleCount;
};

DECLARE_NODE(CubeRadiance)
{
    CubeRadianceBlock* params = (CubeRadianceBlock*)parameters;
    int size = 128 << params->size;
    NodeIndex source = int(evaluation->inputIndices[0]);
    int width, height;
    const int res = GetEvaluationSize(context, source, &width, &height);
    if (params->size == 0 && source.IsValid() && res == EVAL_OK)
    {
        size = width;
    }
    
	int target = int(evaluation->targetIndex);
	SetEvaluationPersistent(context, target, 1);
    if (params->mode == 0)
    {
        // radiance
        SetEvaluationCubeSize(context, target, size, 1);
    }
    else
    {
        // irradiance
        SetEvaluationCubeSize(context, target, size, int(log2(size))+1);
    }
    return EVAL_OK;
}


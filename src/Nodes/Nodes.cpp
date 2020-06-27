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

#include "Evaluators.h"
#include "EvaluationContext.h"

using namespace EvaluationAPI;
std::map<std::string, NodeFunction> nodeFunctions;

struct AddNodeFunction
{
    AddNodeFunction(const std::string& nodeName, NodeFunction function)
    {
        nodeFunctions[nodeName] = function;
    }
};

#define DECLARE_NODE(NODE) int NODE(void *parameters, EvaluationInfo* evaluation, EvaluationContext* context); \
AddNodeFunction add##NODE(#NODE, NODE); \
int NODE(void *parameters, EvaluationInfo* evaluation, EvaluationContext* context)

#include "Crop.h"
#include "CubeRadiance.h"
#include "Distance.h"
#include "EquirectConverter.h"
#include "GradientBuilder.h"
#include "ImageRead.h"
#include "ImageWrite.h"
#include "Paint2D.h"
#include "PhysicalSky.h"
#include "ReactionDiffusion.h"
#include "SVG.h"
#include "Thumbnail.h"


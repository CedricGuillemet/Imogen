// https://github.com/CedricGuillemet/Imogen
//
// The MIT License(MIT)
// 
// Copyright(c) 2018 Cedric Guillemet
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

#pragma once

#include <vector>
struct TileNodeEditGraphDelegate;
struct Evaluation;
class TextEditor;
struct Library;

enum EVALUATOR_TYPE
{
	EVALUATOR_GLSL,
	EVALUATOR_C,
};

struct EvaluatorFile
{
	std::string mDirectory;
	std::string mFilename;
	EVALUATOR_TYPE mEvaluatorType;
};
struct Imogen
{
	Imogen();
	~Imogen();
	
	void Show(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
	void DiscoverNodes(const char *wildcard, const char *directory, EVALUATOR_TYPE evaluatorType, std::vector<EvaluatorFile>& files);

	std::vector<EvaluatorFile> mEvaluatorFiles;

protected:
	void HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
};

void DebugLogText(const char *szText);
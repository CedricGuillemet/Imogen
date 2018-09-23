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
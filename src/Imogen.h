#pragma once

#include <vector>
struct TileNodeEditGraphDelegate;
struct Evaluation;
class TextEditor;

struct Imogen
{
	Imogen();
	~Imogen();
	
	void Show(TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
	void DiscoverShaders();

	std::vector<std::string> shaderFileNames;

protected:
	void HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
};

void DebugLogText(const char *szText);
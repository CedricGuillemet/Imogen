#pragma once

#include <vector>
struct TileNodeEditGraphDelegate;
struct Evaluation;
class TextEditor;
struct Library;

struct Imogen
{
	Imogen();
	~Imogen();
	
	void Show(Library& library, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
	void Imogen::DiscoverNodes(const char *dir, std::vector<std::string>& files);

	std::vector<std::string> shaderFileNames;
	std::vector<std::string> cFileNames;

protected:
	void HandleEditor(TextEditor &editor, TileNodeEditGraphDelegate &nodeGraphDelegate, Evaluation& evaluation);
};

void DebugLogText(const char *szText);
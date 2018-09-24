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

#include "Library.h"

enum : uint32_t
{
	v_initial,
	v_materialComment,
	v_thumbnail,
	v_lastVersion
};
#define ADD(_fieldAdded, _fieldName) if (dataVersion >= _fieldAdded){ Ser(_fieldName); }
#define ADD_LOCAL(_localAdded, _type, _localName, _defaultValue) \
	_type _localName = (_defaultValue); \
	if (dataVersion >= (_localAdded)) { Ser(_localName)); }
#define REM(_fieldAdded, _fieldRemoved, _type, _fieldName, _defaultValue) \
	_type _fieldName = (_defaultValue); \
	if (dataVersion >= (_fieldAdded) && dataVersion < (_fieldRemoved)) { Ser(_fieldName); }
#define VERSION_IN_RANGE(_from, _to) \
	(dataVersion >= (_from) && dataVersion < (_to))

template<bool doWrite> struct Serialize
{
	Serialize(const char *szFilename)
	{
		fp = fopen(szFilename, doWrite ? "wb" : "rb");
	}
	~Serialize()
	{
		if (fp)
			fclose(fp);
	}
	template<typename T> void Ser(T& data)
	{
		if (doWrite)
			fwrite(&data, sizeof(T), 1, fp);
		else
			fread(&data, sizeof(T), 1, fp);
	}
	void Ser(std::string& data)
	{
		if (doWrite)
		{
			uint32_t len = uint32_t(data.length() + 1);
			fwrite(&len, sizeof(uint32_t), 1, fp);
			fwrite(data.c_str(), len, 1, fp);
		}
		else
		{
			uint32_t len;
			fread(&len, sizeof(uint32_t), 1, fp);
			data.resize(len);
			fread(&data[0], len, 1, fp);
		}
	}
	template<typename T> void Ser(std::vector<T>& data)
	{
		uint32_t count = uint32_t(data.size());
		Ser(count);
		data.resize(count);
		for (auto& item : data)
			Ser(&item);
	}
	void Ser(std::vector<uint8_t>& data)
	{
		uint32_t count = uint32_t(data.size());
		Ser(count);
		if (!count)
			return;
		if (doWrite)
		{
			fwrite(data.data(), count, 1, fp);
		}
		else
		{
			data.resize(count);
			fread(&data[0], count, 1, fp);
		}
	}

	void Ser(InputSampler *inputSampler)
	{
		ADD(v_initial, inputSampler->mWrapU);
		ADD(v_initial, inputSampler->mWrapV);
		ADD(v_initial, inputSampler->mFilterMin);
		ADD(v_initial, inputSampler->mFilterMag);
	}
	void Ser(MaterialNode *materialNode)
	{
		ADD(v_initial, materialNode->mType);
		ADD(v_initial, materialNode->mPosX);
		ADD(v_initial, materialNode->mPosY);
		ADD(v_initial, materialNode->mInputSamplers);
		ADD(v_initial, materialNode->mParameters);
	}
	void Ser(MaterialConnection *materialConnection)
	{
		ADD(v_initial, materialConnection->mInputNode);
		ADD(v_initial, materialConnection->mOutputNode);
		ADD(v_initial, materialConnection->mInputSlot);
		ADD(v_initial, materialConnection->mOutputSlot);
	}
	void Ser(Material *material)
	{
		ADD(v_initial, material->mName);
		ADD(v_materialComment, material->mComment);
		ADD(v_initial, material->mMaterialNodes);
		ADD(v_initial, material->mMaterialConnections);
		ADD(v_thumbnail, material->mThumbnail);
	}
	bool Ser(Library *library)
	{
		if (!fp)
			return false;
		if (doWrite)
			dataVersion = v_lastVersion-1;
		Ser(dataVersion);
		if (dataVersion > v_lastVersion)
			return false; // no forward compatibility
		ADD(v_initial, library->mMaterials);
		return true;
	}
	FILE *fp;
	uint32_t dataVersion;
};

typedef Serialize<true> SerializeWrite;
typedef Serialize<false> SerializeRead;

void LoadLib(Library *library, const char *szFilename)
{
	SerializeRead(szFilename).Ser(library);
}

void SaveLib(Library *library, const char *szFilename)
{
	SerializeWrite(szFilename).Ser(library);
}

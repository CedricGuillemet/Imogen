#include "Library.h"

enum : uint32_t
{
	v_initial,
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
		ADD(v_initial, material->mMaterialNodes);
		ADD(v_initial, material->mMaterialConnections);
		ADD(v_initial, material->mParameters);
	}
	bool Ser(Library *library)
	{
		if (!fp)
			return false;
		if (doWrite)
			dataVersion = v_lastVersion;
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

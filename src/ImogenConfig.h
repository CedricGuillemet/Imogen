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
#pragma once


static const uint16_t InvalidNodeIndex = 0xFFFF;
static const uint8_t InvalidSlotIndex = 0xFF;

struct NodeIndex
{
	NodeIndex() : mNodeIndex(InvalidNodeIndex) {}
	/*NodeIndex(int nodeIndex) : mNodeIndex(uint16_t(nodeIndex)) 
	{
		assert(nodeIndex >= 0);
	}*/
	NodeIndex(size_t nodeIndex) : mNodeIndex(uint16_t(nodeIndex)) 
	{
		assert(nodeIndex == InvalidNodeIndex || nodeIndex < InvalidNodeIndex);
	}

	
	
	bool IsValid() const
	{
		return mNodeIndex != InvalidNodeIndex;
	}

	void SetInvalid()
	{
		mNodeIndex = -1;
	}

	operator int() const 
	{ 
		assert(IsValid());
		return int(mNodeIndex); 
	}

	bool operator == (const NodeIndex& other) const { return mNodeIndex == other.mNodeIndex; }

	NodeIndex& operator -- ()
	{
		mNodeIndex --;
		return *this;
	}

	NodeIndex operator -- (int)
	{
		mNodeIndex--;
		return *this;
	}

	NodeIndex& operator ++ ()
	{
		mNodeIndex ++;
		return *this;
	}

	NodeIndex operator ++ (int)
	{
		mNodeIndex++;
		return *this;
	}

private:
	uint16_t mNodeIndex;

	bool operator != (int value) { return true; }
	//bool operator == (int value) { return false; }
};

struct SlotIndex
{
	SlotIndex() : mSlotIndex(InvalidSlotIndex) {}
	SlotIndex(int slotIndex) : mSlotIndex(uint8_t(slotIndex)) {}

	uint8_t mSlotIndex;

	bool IsValid() const
	{
		return mSlotIndex != InvalidSlotIndex;
	}

	operator int() const 
	{ 
		assert(IsValid());
		return int(mSlotIndex);
	}
};


struct RuntimeId
{
	RuntimeId() : mRuntimeId(GetRuntimeId())
	{
	}

	RuntimeId(const RuntimeId& other) : mRuntimeId(other.mRuntimeId)
	{
	}

	bool operator == (RuntimeId& other) const
	{
		return mRuntimeId == other.mRuntimeId;
	}

private:	
	uint32_t mRuntimeId;

	uint32_t GetRuntimeId()
	{
		static uint32_t runtimeId = 10;
		return ++runtimeId;
	}
};
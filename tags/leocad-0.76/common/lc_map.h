#ifndef _LC_MAP_H_
#define _LC_MAP_H_

typedef void* lcHashMapAssoc;

template<class K, class V>
class lcHashMap
{
public:
	struct lcHashMapPair
	{
		const K Key;
		V Value;
		u32 Hash;
		lcHashMapPair* Next;

		lcHashMapPair(const K& KeyVal) : Key(KeyVal) { }
	};

	struct lcHashMapBlock
	{
		lcHashMapBlock* Next;
	};

	lcHashMap()
	{
		mCount = 0;
		mData = NULL;
		mFreeData = NULL;
		mBlocks = NULL;
	}

	~lcHashMap()
	{
		for (lcHashMapPair* Pair = mData; Pair; Pair = Pair->Next)
			Pair->~lcHashMapPair();

		while (mBlocks)
		{
			lcHashMapBlock* Block = mBlocks->Next;
			delete[] mBlocks;
			mBlocks = Block;
		}
	}

	int GetSize() const
	{
		return mCount;
	}

	lcHashMapAssoc GetFirstAssoc() const
	{
		return mData;
	}

	lcHashMapAssoc GetNextAssoc(lcHashMapAssoc Assoc) const
	{
		return ((lcHashMapPair*)Assoc)->Next;
	}

	const K& GetAssocKey(lcHashMapAssoc Assoc) const
	{
		return ((lcHashMapPair*)Assoc)->Key;
	}

	V& GetAssocValue(lcHashMapAssoc Assoc) const
	{
		return ((lcHashMapPair*)Assoc)->Value;
	}

	V& operator[](const K& Key)
	{
		u32 Hash = MakeHash(Key);
		lcHashMapPair* Pair = FindPair(Key, Hash);

		if (Pair == NULL)
		{
			Pair = AllocPair(Key);
			Pair->Hash = Hash;

			Pair->Next = mData;
			mData = Pair;
		}

		return Pair->Value;
	}

protected:
	u32 MakeHash(const K& Key)
	{
		return (u32)((LC_POINTER_TO_INT(Key))>>4);
	}

	lcHashMapPair* AllocPair(const K& Key)
	{
		if (mFreeData == NULL)
		{
			const int BlockCount = 128;

			lcHashMapBlock* Block = (lcHashMapBlock*)new char[sizeof(lcHashMapBlock) + sizeof(lcHashMapPair) * BlockCount];
			Block->Next = mBlocks;
			mBlocks = Block;

			lcHashMapPair* Pair = (lcHashMapPair*)(mBlocks+1);

			for (int i = 0; i < BlockCount; i++)
			{
				Pair->Next = mFreeData;
				mFreeData = Pair;
				Pair++;
			}
		}

		lcHashMapPair* Pair = mFreeData;
		mFreeData = mFreeData->Next;
		mCount++;

		memset(Pair, 0, sizeof(lcHashMapPair));
		new(Pair) lcHashMapPair(Key);

		return Pair;
	}

	lcHashMapPair* FindPair(const K& Key, u32 Hash)
	{
		for (lcHashMapPair* Pair = mData; Pair; Pair = Pair->Next)
		{
			if (Pair->Hash == Hash && Key == Pair->Key)
				return Pair;
		}

		return NULL;
	}

	int mCount;
	lcHashMapPair* mData;
	lcHashMapPair* mFreeData;
	lcHashMapBlock* mBlocks;
};

#endif // _LC_MAP_H_

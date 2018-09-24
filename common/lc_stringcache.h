#pragma once

struct lcStringCacheEntry
{
	int Left, Right, Top, Bottom;
};

class lcStringCache
{
public:
	lcStringCache();
	~lcStringCache();

	void AddRef(lcContext* Context);
	void Release(lcContext* Context);

	void CacheStrings(lcContext* Context, const QStringList& Strings);
	void GetStringDimensions(int* cx, int* cy, const QString& String) const;
	void DrawStrings(lcContext* Context, const lcMatrix44* Transforms, const QStringList& Strings) const;

protected:
	lcTexture* mTexture;
	unsigned char* mBuffer;
	int mRefCount;

	std::map<QString, lcStringCacheEntry> mStrings;
};

extern lcStringCache gStringCache;

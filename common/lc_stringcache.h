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

	void Initialize(lcContext* Context);
	void Reset();

	void CacheStrings(const QStringList& Strings);
	void GetStringDimensions(int* cx, int* cy, const QString& String) const;
	void DrawStrings(lcContext* Context, const lcMatrix44* Transforms, const QStringList& Strings) const;

protected:
	lcTexture* mTexture;

	std::map<QString, lcStringCacheEntry> mStrings;
};

extern lcStringCache gStringCache;

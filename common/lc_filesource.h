#pragma once

struct lcEmbeddedData;

// A resolved disk path, archive entry, or embedded file. Copying it does not
// snapshot file contents; workers open their own file from the copied source.
struct lcFileSource
{
	bool operator==(const lcFileSource& Other) const
	{
		return EmbeddedData == Other.EmbeddedData && FilePath == Other.FilePath && ArchiveType == Other.ArchiveType && ArchiveIndex == Other.ArchiveIndex;
	}

	bool operator!=(const lcFileSource& Other) const
	{
		return !(*this == Other);
	}

	std::shared_ptr<const lcEmbeddedData> EmbeddedData;
	QString FilePath;
	int ArchiveType = -1;
	int ArchiveIndex = -1;
};

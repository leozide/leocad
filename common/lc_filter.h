#pragma once

class lcFilter
{
public:
	lcFilter(const std::string_view FilterString);
	~lcFilter() = default;

	bool Match(const char* String) const;

protected:
	static bool FastWildCompare(const char* Tame, const char* Wild);

	enum class FilterOp
	{
		And,
		AndNot,
		Or,
		OrNot
	};

	struct FilterPart
	{
		FilterOp Op;
		bool Wildcard;
		std::string String;
	};

	std::vector<FilterPart> mFilterParts;
};

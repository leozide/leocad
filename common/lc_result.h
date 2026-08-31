#pragma once

// We can't use std::expected because we only require C++17.

#include <stdexcept>

class lcUnexpected
{
public:
	explicit lcUnexpected(const QString& Error)
	    : mError(Error)
	{
	}

	explicit lcUnexpected(QString&& Error)
	    : mError(std::move(Error))
	{
	}

	const QString& error() const
	{
		return mError;
	}
	
	QString& error()
	{
		return mError;
	}
	
protected:
	QString mError;
};

template <typename T>
class [[nodiscard]] lcResult
{
	using internal_t = std::conditional_t<std::is_void_v<T>, char, T>;

public:
	template <typename U = T, typename = std::enable_if_t<!std::is_void_v<U> && std::is_convertible_v<U, T>>>
	lcResult(U&& Value)
	    : mStorage(std::in_place_index<0>, std::forward<U>(Value))
	{
	}

	template <typename U = T, typename = std::enable_if_t<std::is_void_v<U>>>
	lcResult()
	    : mStorage(std::in_place_index<0>)
	{
	}

	lcResult(lcUnexpected&& Unexpected)
	    : mStorage(std::in_place_index<1>, std::move(Unexpected))
	{
	}

	lcResult(const lcResult& Other) = default;
	lcResult& operator=(const lcResult& Other) = default;

	lcResult(lcResult&& Other) = default;
	lcResult& operator=(lcResult&& Other) = default;

	~lcResult() = default;

	bool has_value() const
	{
		return mStorage.index() == 0;
	}

	explicit operator bool() const
	{
		return has_value();
	}

	template <typename U = T, typename = std::enable_if_t<!std::is_void_v<U> && std::is_same_v<U, T>>>
	constexpr const U& value() const
	{
		if (!has_value())
			throw std::logic_error("bad lcResult access");
		
		return std::get<0>(mStorage);
	}
	
	constexpr const QString& error() const
	{
		if (mStorage.index() != 1)
			throw std::logic_error("bad lcResult access");

		return std::get<1>(mStorage).error();
	}

protected:
	std::variant<internal_t, lcUnexpected> mStorage;
};

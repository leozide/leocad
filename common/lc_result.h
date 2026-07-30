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
	    : mHasValue(true)
	{
		::new (static_cast<void*>(&mValue)) internal_t(std::forward<U>(Value));
	}
	
	template <typename U = T, typename = std::enable_if_t<std::is_void_v<U>>>
	lcResult()
	    : mHasValue(true)
	{
	}
	
	lcResult(lcUnexpected Unexpected)
	    : mHasValue(false)
	{
		::new (static_cast<void*>(&mUnexpected)) lcUnexpected(std::move(Unexpected.error()));
	}
	
	~lcResult() noexcept
	{
		if (mHasValue)
		{
			if constexpr (!std::is_void_v<T>)
				mValue.~internal_t();
		}
		else
		{
			mUnexpected.~lcUnexpected();
		}
	}
	
	lcResult(const lcResult& Other) :
	    mHasValue(Other.mHasValue)
	{
		if (mHasValue)
		{
			if constexpr (!std::is_void_v<T>)
				::new (static_cast<void*>(&mValue)) internal_t(Other.mValue);
		}
		else
		{
			::new (static_cast<void*>(&mUnexpected)) lcUnexpected(Other.mUnexpected);
		}
	}
	
	lcResult(lcResult&& Other) noexcept(std::is_nothrow_move_constructible_v<internal_t> && std::is_nothrow_move_constructible_v<lcUnexpected>) 
	    : mHasValue(Other.mHasValue)
	{
		if (mHasValue)
		{
			if constexpr (!std::is_void_v<T>)
				::new (static_cast<void*>(&mValue)) internal_t(std::move(Other.mValue));
		}
		else
		{
			::new (static_cast<void*>(&mUnexpected)) lcUnexpected(std::move(Other.mUnexpected));
		}
	}
	
	bool has_value() const
	{
		return mHasValue;
	}

	explicit operator bool() const
	{
		return has_value();
	}

	template <typename U = T, typename = std::enable_if_t<!std::is_void_v<U>>>
	constexpr const U& value() const
	{
		if (!mHasValue)
			throw std::logic_error("bad lcResult access");
		
		return mValue;
	}
	
	constexpr const QString& error() const
	{
		if (mHasValue)
			throw std::logic_error("bad lcResult access");

		return mUnexpected.error();
	}

protected:
	union
	{
		internal_t mValue;
		lcUnexpected mUnexpected;
	};
	bool mHasValue;
};

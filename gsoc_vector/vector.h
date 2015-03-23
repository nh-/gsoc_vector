
#pragma once

#include <cstddef>
#include <malloc.h>
#include <exception>
#include <vector>
#include <cassert>

#ifdef _MSC_VER
#define FCV_NOEXCEPT _NOEXCEPT
#else
#error "TODO!"
#endif
#define FCV_CONSTEXPR




template<
	typename _Ty,
	typename _Alloc = std::allocator<_Ty>
>
class fixed_capacity_vector
{
public:
	typedef _Ty value_type;
	typedef _Alloc allocator_type;
	typedef unsigned int size_type;

	explicit fixed_capacity_vector(size_type _capacity, const allocator_type& allocator = allocator_type())
		: capacity_(0), allocator_(allocator), buffer_(nullptr), size_(0)
	{
		_alloc(_capacity);
	}

	fixed_capacity_vector(const fixed_capacity_vector& other)
		: capacity_(0), buffer_(nullptr), size_(0)
		, allocator_(std::allocator_traits<allocator_type>::select_on_container_copy_construction(other.allocator_)) 
	{
		_alloc(other.capacity());
		_copy_construct(other.size(), other.buffer_);
	}

	fixed_capacity_vector& operator=(const fixed_capacity_vector& other)
	{
		if(this != &other)
		{
			if(std::allocator_traits<allocator_type>::propagate_on_container_copy_assignment::value
				&& allocator_ != other.allocator_)
			{
				clear();
				_free();
				allocator_ = other.allocator_;
				_alloc(other.capacity());
			}
			else if(capacity_ != other.capacity())
			{
				clear();
				_free();
				_alloc(other.capacity());
			}

			if(size() >= other.size())
			{
				resize(other.size());
				_copy_assign(other.size(), buffer_, other.buffer_);
			}
			else
			{
				_copy_assign(size(), buffer_, other.buffer_);
				_copy_construct(other.size() - size(), other.buffer_ + size());
			}
		}
		return *this;
	}

	~fixed_capacity_vector() FCV_NOEXCEPT
	{
		clear();
		_free();
	}

	size_type capacity() const FCV_NOEXCEPT 
	{
		return capacity_;
	}
		
	size_type size() const FCV_NOEXCEPT
	{
		return size_;
	}

	void resize(size_type _size, const value_type& value = value_type())
	{
		if(_size > capacity_)
			throw std::length_error("size exceeds capacity of fixed_capacity_vector");

		// shrink if new size is < current size
		for(; size() > _size; )
			pop_back();

		// grow if new size is > current size
		for(; size_ < _size; ++size_) // increment size after construction because constructors may throw
			_emplace(buffer_ + size_, value);
	}
		
	void push_back(const value_type& value)
	{
		if(size_ == capacity_)
			throw std::length_error("fixed_capacity_vector out of capacity");

		_emplace(buffer_ + size_, value);
		// modify size after copy to be consistent if copy-constructor throws
		++size_;
	}

	void push_back(value_type&& value)
	{
		if(size_ == capacity_)
			throw std::length_error("fixed_capacity_vector out of capacity");

		_emplace(buffer_ + size_, std::move(value));
		// modify size after move to be consistent if move-constructor throws
		++size_;
	}

	void pop_back()
	{
		// TODO: 
		{

			// destructors should not throw but in case one does, we still stay
			// in a consistent state if we decrement size first
			--size_; 
			_destroy(buffer_ + size_);
		}
	}

	void clear()
	{
		for(; size(); )
			pop_back();
	}

	value_type& at(size_type index)
	{
		assert(index < size_);
		return *(buffer_ + index);
	}

	const value_type& at(size_type index) const
	{
		assert(index < size_);
		return *(buffer_ + index);
	}

	value_type& operator[](size_type index)
	{
		return at(index);
	}

	const value_type& operator[](size_type index) const
	{
		return at(index);
	}
	
	value_type* data() FCV_NOEXCEPT
	{
		return buffer_;
	}

	const value_type* data() const FCV_NOEXCEPT
	{
		return buffer_;
	}
	
private:
	void _alloc(size_type _capacity)
	{
		assert(!buffer_);
		buffer_ = _capacity ? std::allocator_traits<allocator_type>::allocate(allocator_, _capacity) : nullptr;
		capacity_ = _capacity;
	}

	void _free()
	{
		if(capacity_)
		{
			assert(buffer_);
			std::allocator_traits<allocator_type>::deallocate(allocator_, buffer_, capacity_);
			capacity_ = 0;
			buffer_ = nullptr;
		}
	}

	void _copy_assign(size_type n, value_type* dst, const value_type* src)
	{
		for(; n > 0; --n)
			*dst++ = *src++;
	}

	void _copy_construct(size_type n, const value_type* src)
	{
		assert(buffer_);
		assert(size() + n <= capacity());
		for(value_type* p = buffer_ + size(); n>0; --n, ++size_)
			_emplace(p++, *src++);
// 
// 		for(; n > 0; --n)
// 			_emplace(dst++, *src++);
	}

	template<
		typename... _TyArgs
	>
	void _emplace(value_type* dst, _TyArgs&&... args)
	{
		assert(buffer_);
		assert(dst < (buffer_ + capacity_));
		std::allocator_traits<allocator_type>::construct(allocator_, dst, std::forward<_TyArgs>(args)...);
	}

	void _destroy(value_type* dst)
	{
		// TODO: 
	}

private:
	// hottest stuff goes first
	value_type* buffer_;
	size_type size_;
	size_type capacity_;
	allocator_type allocator_;
};
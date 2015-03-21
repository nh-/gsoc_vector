
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

void* fcv_aligned_malloc(std::size_t alignment, std::size_t size)
{
	auto p = _aligned_malloc(size, alignment);
	if(!p) throw std::bad_alloc();
	return p;
}

void fcv_aligned_free(void* p)
{
	_aligned_free(p);
}


/*
template<typename>
class DummyAlloc {};

struct ConstructionCapacity
{
	//typedef std::size_t size_type;
	typedef unsigned int size_type;

	ConstructionCapacity(size_type capacity) FCV_NOEXCEPT
		: capacity_(capacity)
	{
	}

	FCV_CONSTEXPR size_type capacity() FCV_NOEXCEPT { return capacity_; }
	
private:
	size_type capacity_;
	
};
*/


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

	fixed_capacity_vector(size_type _capacity)
		: capacity_(_capacity), buffer_(nullptr)
	{
		buffer_ = allocator_.allocate(capacity_);
	}

	~fixed_capacity_vector() FCV_NOEXCEPT
	{
		for(size_type i=0; i<size_; ++i)
			allocator_.destroy(buffer_ + i);
		allocator_.deallocate(buffer_, capacity_);
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
			throw std::length_error("size would exceed capacity of fixed_capacity_vector");

		// shrink if new size is < current size
		for(; size_ > _size; )
		{
			// destructors should not throw but in case one does, we still stay
			// in a consistent state if we decrement size first
			--size_; 
			allocator_.destroy(buffer_ + size_);
		}

		// grow if new size is > current size
		for(; size_ < _size; ++size_)
		{
			allocator_.construct(buffer_ + size_, value);
			// increment size after construction because constructors may throw
		}
	}
		
	void push_back(const value_type& value)
	{
		if(size_ == capacity_)
			throw std::length_error("fixed_capacity_vector out of capacity");

		allocator_.construct(buffer_ + size_, value);
		// modify size after copy to be consistent if copy-constructor throws
		++size_;
	}

	void push_back(value_type&& value)
	{
		if(size_ == capacity_)
			throw std::length_error("fixed_capacity_vector out of capacity");

		allocator_.construct(buffer_ + size_, std::move(value));
		// modify size after move to be consistent if move-constructor throws
		++size_;
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
	const size_type capacity_;
	allocator_type allocator_;

	value_type* buffer_;
	size_type size_;
};
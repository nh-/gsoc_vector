
#pragma once

#include <cstddef>
#include <malloc.h>
#include <exception>

#define FCV_NOEXCEPT 
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
class fixed_capacity_vector //: public _CapacityPolicy
{
public:
	typedef _Ty value_type;
	typedef _Alloc allocator_type;
	typedef unsigned int size_type;

	fixed_capacity_vector(size_type _capacity)
		: capacity_(_capacity)
	{
		buffer_ = static_cast<value_type*>(fcv_aligned_malloc(std::alignment_of<value_type>::value, 
			capacity() * sizeof(value_type)));
	}

	~fixed_capacity_vector()
	{
		fcv_aligned_free(buffer_);
	}

	size_type capacity() const FCV_NOEXCEPT 
	{
		return capacity_;
	}
		
	size_type size() const FCV_NOEXCEPT
	{
		return size_;
	}



private:
	const size_type capacity_;
	value_type* buffer_;
	size_type size_;
};
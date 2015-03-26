
#pragma once

#include <memory>

struct AllocatorCallStatistics
{
	int AllocateCalls;
	int DeallocateCalls;
	int ConstructCalls;
	int DestroyCalls;

	AllocatorCallStatistics(int allocateCalls = 0, int deallocateCalls = 0,
		int constructCalls = 0, int destroyCalls = 0)
		: AllocateCalls(allocateCalls), DeallocateCalls(deallocateCalls)
		, ConstructCalls(constructCalls), DestroyCalls(destroyCalls)
	{
	}
};

bool operator==(const AllocatorCallStatistics& lhs, const AllocatorCallStatistics& rhs)
{
	return lhs.AllocateCalls == rhs.AllocateCalls
		&& lhs.DeallocateCalls == rhs.DeallocateCalls
		&& lhs.ConstructCalls == rhs.ConstructCalls
		&& lhs.DestroyCalls == rhs.DestroyCalls;
}
bool operator!=(const AllocatorCallStatistics& lhs, const AllocatorCallStatistics& rhs)
{
	return !operator==(lhs, rhs);
}

template<
	typename T
>
class AllocatorMock
{
public:
	typedef AllocatorCallStatistics Statistics;

	typedef T value_type;

	template<typename U> 
	struct rebind { typedef AllocatorMock<U> other; };

	AllocatorMock(Statistics* stats = nullptr) 
		: stats_(stats), alloc_()
	{
	}
	template<typename U> 
	AllocatorMock(const AllocatorMock<U>& other)
		: stats_(other.stats_), alloc_(other.alloc_)
	{
	}

	value_type* allocate(std::size_t n)
	{
		if(stats_) 
			++stats_->AllocateCalls;
		return alloc_.allocate(n);
	}
	void deallocate(value_type* p, std::size_t n)
	{
		if(stats_)
			++stats_->DeallocateCalls;
		alloc_.deallocate(p, n);
	}
	template<typename U, typename... Args>
	void construct(U* p, Args&&... args)
	{
		if(stats_)
			++stats_->ConstructCalls;
		alloc_.construct(p, std::forward<Args>(args)...);
	}
	void destroy(value_type* p)
	{
 		if(stats_)
 			++stats_->DestroyCalls;
		alloc_.destroy(p);
	}
	
	AllocatorMock select_on_container_copy_construction() const
	{
		return AllocatorMock(stats_);
	}
	
	Statistics* stats_;
	std::allocator<value_type> alloc_;
};


template<typename T>
bool operator==(const AllocatorMock<T>& lhs, const AllocatorMock<T>& rhs)
{
	return lhs.stats_ == rhs.stats_;
}
template<typename T>
bool operator!=(const AllocatorMock<T>& lhs, const AllocatorMock<T>& rhs)
{
	return !operator==(lhs, rhs);
}




#include "vector.h"
#include <vector>
#include <array>
#include <gtest/gtest.h>


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
AllocatorCallStatistics operator-(const AllocatorCallStatistics& lhs, const AllocatorCallStatistics& rhs)
{
	return AllocatorCallStatistics(lhs.AllocateCalls - rhs.AllocateCalls,
		lhs.DeallocateCalls - rhs.DeallocateCalls,
		lhs.ConstructCalls - rhs.ConstructCalls,
		lhs.DestroyCalls - rhs.DestroyCalls);
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
	
private:
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


struct NonDefaultConstructable
{
	NonDefaultConstructable(int x)
		: x_(x)
	{
	}

	int x_;
};

struct NonCopyable
{
	NonCopyable() = default;
private:
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
public:
	NonCopyable(NonCopyable&& other) { }
	NonCopyable& operator=(NonCopyable&& other) { return *this; }
};

template<typename T>
T construct(int id = 0) { return T(id); }
// template<>
// int construct<int>(int id) { return id; }
template<>
std::string construct<std::string>(int id) { return std::string(id, '~'); }
template<>
std::pair<short, short> construct<std::pair<short, short>>(int id) { return std::make_pair(id, id); }

// for types that are not default-constructible
// template<>
// NonDefaultConstructable construct<NonDefaultConstructable>()
// {
// 	return NonDefaultConstructable(1337);
// }


template<typename T>
class fcv_basic_test : public ::testing::Test
{
public:
};

typedef ::testing::Types<
	fixed_capacity_vector<int, AllocatorMock<int>>, 
 	fixed_capacity_vector<void*, AllocatorMock<void*>>,
 	fixed_capacity_vector<const void*, AllocatorMock<const void*>>,
	fixed_capacity_vector<std::string, AllocatorMock<std::string>>,
 	fixed_capacity_vector<std::pair<short, short>, AllocatorMock<std::pair<short, short>>>,
	fixed_capacity_vector<std::vector<char>, AllocatorMock<std::vector<char>>>
// 	fixed_capacity_vector<NonDefaultConstructable, AllocatorMock<NonDefaultConstructable>>
// 	fixed_capacity_vector<NonCopyable>
> TypesUnderTest;
TYPED_TEST_CASE(fcv_basic_test, TypesUnderTest);


TYPED_TEST(fcv_basic_test, ctor_and_capacity)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	for(auto c : { 0, 1, 2, 3, 7, 19, 23, 100, 1000, 1234, 1337, 8000 })
	{
		stats_t stats;
		{
			const TypeParam myvec(c, alloc_t(&stats));
			ASSERT_EQ(c == 0 ? stats_t(0, 0, 0, 0) : stats_t(1, 0, 0, 0), stats);
			ASSERT_EQ(c, myvec.capacity());
		}
		ASSERT_EQ(c == 0 ? stats_t(0, 0, 0, 0) : stats_t(1, 1, 0, 0), stats);
	}
}

TYPED_TEST(fcv_basic_test, size)
{
	// testing size is not really possible without using functions like push_back and resize
	// so we only test for size() = 0 on empty vector and for size() beeing callable on const objects
	const std::size_t c = 8;
	const TypeParam myvec(c);

	ASSERT_EQ(0, myvec.size());
	ASSERT_EQ(true, myvec.empty());
}

TYPED_TEST(fcv_basic_test, resize)
{	
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	ASSERT_EQ(0, myvec.size());

	std::array<TypeParam::value_type, c> expected;

	// test valid sizes (shrinking and growing)
	auto data = myvec.data();
	for(auto s : { 3, 7, 1, 8, 0, 4 })
	{
		// store current values in expected
		auto oldSize = myvec.size();
		for(std::size_t i=0; i<oldSize; ++i)
			expected[i] = data[i];
	
		// resize and fill with new size as value
		ASSERT_NO_THROW(myvec.resize(s, construct<TypeParam::value_type>(s)));
		
		// check correct size
		auto newSize = myvec.size();
		ASSERT_EQ(s, newSize);

		// check allocator calls
		if(oldSize < newSize)
			expectedStats.ConstructCalls += (newSize - oldSize);
		else if(oldSize > newSize)
		{
			expectedStats.DestroyCalls += 
				std::is_scalar<TypeParam::value_type>::value ? 0 : (oldSize - newSize);
		}
		ASSERT_EQ(expectedStats, stats);

		// check unchanged elements
		ASSERT_EQ(true, std::equal(data, data + std::min(oldSize, newSize), begin(expected)));
		// check new elements
 		for(std::size_t i=oldSize; i<newSize; ++i)
 			ASSERT_EQ(construct<TypeParam::value_type>(s), data[i]);
	}

	// test resize with too large size
	auto oldSize = myvec.size();
	std::copy(data, data + oldSize, begin(expected));
	ASSERT_THROW(myvec.resize(c + 1), std::length_error);
	auto newSize = myvec.size();
	// check that size has not changed
 	ASSERT_EQ(oldSize, newSize);
 	// check for consistent content
 	ASSERT_EQ(true, std::equal(data, data + newSize, begin(expected)));
	// check for no calls to allocator mock
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, push_back_copy)
{	
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));

	std::array<TypeParam::value_type, c> expected;

	// cases where vector is not full
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<TypeParam::value_type>(i);

		ASSERT_NO_THROW(myvec.push_back(expected[i]));
		ASSERT_EQ(i+1, myvec.size());

		// check for allocator mock calls
		++expectedStats.ConstructCalls;
		ASSERT_EQ(expectedStats, stats);

		// check for expected content
		auto data = myvec.data();
		ASSERT_EQ(true, std::equal(data, data + i + 1, begin(expected)));
	}

	// cases where vector is full
	const auto value = construct<TypeParam::value_type>(c);
	ASSERT_THROW(myvec.push_back(value), std::length_error);
	// check that size, capacity and content is unchanged
	ASSERT_EQ(c, myvec.capacity());
	ASSERT_EQ(c, myvec.size());
	auto data = myvec.data();
	ASSERT_EQ(true, std::equal(data, data + c, begin(expected)));
	// check for no calls to allocator in case of thrown exceptions
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, push_back_move)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));

	std::array<TypeParam::value_type, c> expected;

	// cases where vector is not full
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<TypeParam::value_type>(i);

		ASSERT_NO_THROW(myvec.push_back(construct<TypeParam::value_type>(i)));
		ASSERT_EQ(i+1, myvec.size());

		// check for allocator mock calls
		++expectedStats.ConstructCalls;
		ASSERT_EQ(expectedStats, stats);

		// check for expected content
		auto data = myvec.data();
		ASSERT_EQ(true, std::equal(data, data + i + 1, begin(expected)));
	}

	// cases where vector is full
	ASSERT_THROW(myvec.push_back(construct<TypeParam::value_type>(c)), std::length_error);
	// check that size, capacity and content is unchanged
	ASSERT_EQ(c, myvec.capacity());
	ASSERT_EQ(c, myvec.size());
	auto data = myvec.data();
	ASSERT_EQ(true, std::equal(data, data + c, begin(expected)));
	// check for no calls to allocator in case of thrown exception
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, dtor)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	std::size_t size;
	{
		TypeParam myvec(c, alloc_t(&stats));
		myvec.resize(c/2);
		expectedStats.ConstructCalls += (c/2);
		ASSERT_EQ(expectedStats, stats);
		size = myvec.size();
	}
	expectedStats.DestroyCalls += 
		std::is_scalar<TypeParam::value_type>::value ? 0 : size;
	++expectedStats.DeallocateCalls;
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, clear)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	myvec.resize(c/2);
	expectedStats.ConstructCalls += (c/2);
	ASSERT_EQ(expectedStats, stats);

	auto size = myvec.size();
	myvec.clear();
	expectedStats.DestroyCalls +=
		std::is_scalar<TypeParam::value_type>::value ? 0 : size;
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, copy_constructor)
{	
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	for(std::size_t c : { 0, 8 })
	{
		stats_t stats, expectedStats(c ? 1 : 0, 0, 0, 0);
		{
			TypeParam myvec(c, alloc_t(&stats));
			// fill src vector
			for(std::size_t i=0; i<c; ++i)
				myvec.push_back(construct<TypeParam::value_type>(i));
			expectedStats.ConstructCalls += c;
			ASSERT_EQ(expectedStats, stats);

			// copy vector
			TypeParam myvec2(myvec);
			expectedStats.AllocateCalls *= 2;
			expectedStats.ConstructCalls *= 2;

			// check capacity, size and content
			ASSERT_EQ(myvec.capacity(), myvec2.capacity());
			ASSERT_EQ(myvec.size(), myvec2.size());
			auto data = myvec.data();
			auto data2 = myvec2.data();
			for(std::size_t i=0; i<myvec.size(); ++i)
				ASSERT_EQ(data[i], data2[i]);
			// check allocator calls
			ASSERT_EQ(expectedStats, stats);
		}

		expectedStats.DestroyCalls = std::is_scalar<TypeParam::value_type>::value 
			? 0 : expectedStats.ConstructCalls;
		expectedStats.DeallocateCalls = c ? 2 : 0;
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, move_constructor)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	for(std::size_t c : { 0, 8 })
	{
		stats_t stats, expectedStats(c ? 1 : 0, 0, 0, 0);
		{
			TypeParam myvec(c, alloc_t(&stats));
			// fill src vector
			for(std::size_t i=0; i<c; ++i)
				myvec.push_back(construct<TypeParam::value_type>(i));
			expectedStats.ConstructCalls += c;
			ASSERT_EQ(expectedStats, stats);

			// copy vector
			TypeParam myvec2(std::move(myvec));

			// check capacity, size and content of old vector
			ASSERT_EQ(0, myvec.capacity());
			ASSERT_EQ(0, myvec.size());
			ASSERT_EQ(nullptr, myvec.data());

			// check capacity, size and content of new vector
			ASSERT_EQ(c, myvec2.capacity());
			ASSERT_EQ(c, myvec2.size());
			auto data2 = myvec2.data();
			for(std::size_t i=0; i<myvec2.size(); ++i)
				ASSERT_EQ(construct<TypeParam::value_type>(i), data2[i]);

			// check allocator calls
			ASSERT_EQ(expectedStats, stats);
		}

		expectedStats.DestroyCalls = std::is_scalar<TypeParam::value_type>::value 
			? 0 : expectedStats.ConstructCalls;
		expectedStats.DeallocateCalls = c ? 1 : 0;
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, initlist_constructor)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	std::initializer_list<TypeParam::value_type> il{
		construct<TypeParam::value_type>(0),
		construct<TypeParam::value_type>(1),
		construct<TypeParam::value_type>(2),
		construct<TypeParam::value_type>(3)
	};

	stats_t stats, expectedStats(0, 0, 0, 0);

	for(std::size_t c : { 4, 8, 2, 0 })
	{
		if(c >= il.size())
		{
			TypeParam myvec(c, il, alloc_t(&stats));
			expectedStats.AllocateCalls += (c ? 1 : 0);
			expectedStats.ConstructCalls += il.size();
			// check capacity, size and content
			ASSERT_EQ(c, myvec.capacity());
			ASSERT_EQ(il.size(), myvec.size());
			auto data = myvec.data();
			for(std::size_t i=0; i<myvec.size(); ++i)
				ASSERT_EQ(*(begin(il)+i), data[i]);
			// check allocator mock
			ASSERT_EQ(expectedStats, stats);

			// track destructor calls
			expectedStats.DestroyCalls += 
				std::is_scalar<TypeParam::value_type>::value ? 0 : myvec.size();
			expectedStats.DeallocateCalls += (c ? 1 : 0);
		}
		else
		{
			ASSERT_THROW(TypeParam myvec(c, il, alloc_t(&stats)), std::length_error);
			ASSERT_EQ(expectedStats, stats);
		}
	}
}

TYPED_TEST(fcv_basic_test, copy_assign)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));

	for(std::size_t i=0; i<(c/2); ++i)
		myvec.push_back(construct<TypeParam::value_type>(i));
	expectedStats.ConstructCalls += (c/2);

	// assign vector of same capacity (smaller = larger)
	{
		TypeParam myvec2(c, alloc_t(&stats));
		myvec2.push_back(construct<TypeParam::value_type>());
		++expectedStats.AllocateCalls;
		++expectedStats.ConstructCalls;
		ASSERT_EQ(expectedStats, stats);
		auto oldSize = myvec2.size();

		myvec2 = myvec;
		// check capacity, size and content
		ASSERT_EQ(myvec.capacity(), myvec2.capacity());
		ASSERT_EQ(myvec.size(), myvec2.size());
		auto data = myvec.data();
		auto data2 = myvec2.data();
		for(std::size_t i=0; i<myvec.size(); ++i)
			ASSERT_EQ(data[i], data2[i]);
		// check allocator calls
		expectedStats.ConstructCalls += (myvec.size() - oldSize);
		ASSERT_EQ(expectedStats, stats);

		// track stats for destruction of myvec2
		expectedStats.DestroyCalls += 
			std::is_scalar<TypeParam::value_type>::value ? 0 : myvec2.size();
		++expectedStats.DeallocateCalls;
	}

	// assign vector of same capacity (larger = smaller)
	{
		TypeParam myvec2(c, alloc_t(&stats));
		myvec2.resize(c);
		++expectedStats.AllocateCalls;
		expectedStats.ConstructCalls += c;
		ASSERT_EQ(expectedStats, stats);
		auto oldSize = myvec2.size();

		myvec2 = myvec;
		// check capacity, size and content
		ASSERT_EQ(myvec.capacity(), myvec2.capacity());
		ASSERT_EQ(myvec.size(), myvec2.size());
		auto data = myvec.data();
		auto data2 = myvec2.data();
		for(std::size_t i=0; i<myvec.size(); ++i)
			ASSERT_EQ(data[i], data2[i]);
		// check allocator calls (no allocation, deallocation, we just destroy what is too much)
		expectedStats.DestroyCalls += std::is_scalar<TypeParam::value_type>::value 
			? 0 : (oldSize - myvec2.size());
		ASSERT_EQ(expectedStats, stats);
		
		// track stats for destruction of myvec2
		expectedStats.DestroyCalls += std::is_scalar<TypeParam::value_type>::value 
			? 0 : myvec2.size();
		++expectedStats.DeallocateCalls;
	}

	// assign vector of different capacity
	{
		TypeParam myvec2(c + 3, alloc_t(&stats));
		myvec2.push_back(construct<TypeParam::value_type>());
		++expectedStats.AllocateCalls;
		++expectedStats.ConstructCalls;
		ASSERT_EQ(expectedStats, stats);
		auto oldSize = myvec2.size();

		myvec2 = myvec;
		// check capacity, size and content
		ASSERT_EQ(myvec.capacity(), myvec2.capacity());
		ASSERT_EQ(myvec.size(), myvec2.size());
		auto data = myvec.data();
		auto data2 = myvec2.data();
		for(std::size_t i=0; i<myvec.size(); ++i)
			ASSERT_EQ(data[i], data2[i]);
		// check allocator calls
		expectedStats.DestroyCalls += 
			std::is_scalar<TypeParam::value_type>::value ? 0 : oldSize;
		++expectedStats.DeallocateCalls;
		++expectedStats.AllocateCalls;
		expectedStats.ConstructCalls += myvec2.size();
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, move_assign)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(0, 0, 0, 0);
	TypeParam myvec(0, alloc_t(&stats));
	std::array<TypeParam::value_type, 8> expected;
	for(std::size_t i=0; i<8; ++i)
		expected[i] = construct<TypeParam::value_type>(i);

	// 1. nonempty to empty
	// 2. nonempty to nonempty
	// 3. empty to nonempty
	for(std::size_t c : { 8, 6, 0 })
	{
		TypeParam myvec2(c, alloc_t(&stats));
		for(std::size_t i=0; i<(c/2); ++i)
			myvec2.push_back(expected[i]);
		expectedStats.AllocateCalls += (c ? 1 : 0);
		expectedStats.ConstructCalls += (c/2);
		ASSERT_EQ(expectedStats, stats);

		auto oldCapacity = myvec.capacity();
		auto oldSize = myvec.size();
		myvec = std::move(myvec2);
		expectedStats.DestroyCalls += 
			std::is_scalar<TypeParam::value_type>::value ? 0 : oldSize;
		expectedStats.DeallocateCalls += (oldCapacity ? 1 : 0);
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, initlist_assign)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	std::initializer_list<TypeParam::value_type> empty_il {};
	std::initializer_list<TypeParam::value_type> small_il { construct<TypeParam::value_type>(0),  
		construct<TypeParam::value_type>(1), construct<TypeParam::value_type>(2) };
	std::initializer_list<TypeParam::value_type> medium_il { construct<TypeParam::value_type>(0),  
		construct<TypeParam::value_type>(1), construct<TypeParam::value_type>(2),  
		construct<TypeParam::value_type>(3), construct<TypeParam::value_type>(4) };
	std::initializer_list<TypeParam::value_type> large_il { construct<TypeParam::value_type>(0),  
		construct<TypeParam::value_type>(1), construct<TypeParam::value_type>(2),  
		construct<TypeParam::value_type>(3), construct<TypeParam::value_type>(4),  
		construct<TypeParam::value_type>(5), construct<TypeParam::value_type>(6) };
	
	std::array<TypeParam::value_type, 8> expected;

	for(std::size_t c : { 0, 8 })
	{
		stats_t stats, expectedStats(c ? 1 : 0, 0, 0, 0);
		TypeParam myvec(c, alloc_t(&stats));

		for(const auto& il : { empty_il, medium_il, small_il, large_il, empty_il })
		{
			if(il.size() > c)
			{
				auto oldSize = myvec.size();
				for(std::size_t i=0; i<oldSize; ++i)
					expected[i] = myvec.data()[i];

				ASSERT_EQ(expectedStats, stats);
				ASSERT_THROW(myvec = il, std::length_error);
				// check capacity, size and content is unchanged
				ASSERT_EQ(c, myvec.capacity());
				ASSERT_EQ(oldSize, myvec.size());
				for(std::size_t i=0; i<myvec.size(); ++i)
					ASSERT_EQ(expected[i], myvec.data()[i]);
				// check no allocator mock calls have appeared
				ASSERT_EQ(expectedStats, stats);
			}
			else
			{
				auto oldSize = myvec.size();
				ASSERT_NO_THROW(myvec = il);
				if(oldSize > il.size())
				{
					expectedStats.DestroyCalls += 
						std::is_scalar<TypeParam::value_type>::value ? 0 : (oldSize - il.size());
				}
				if(oldSize < il.size())
					expectedStats.ConstructCalls += (il.size() - oldSize);

				// check capacity, size and content
				ASSERT_EQ(c, myvec.capacity());
				ASSERT_EQ(il.size(), myvec.size());
				auto data = myvec.data();
				for(std::size_t i=0; i<il.size(); ++i)
					ASSERT_EQ(*(begin(il) + i), *(data + i));
				// check allocator mock calls
				ASSERT_EQ(expectedStats, stats);
			}
		}
	}
}

TYPED_TEST(fcv_basic_test, at)
{	
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));	
	myvec.resize(c);
	expectedStats.ConstructCalls += c;
	ASSERT_EQ(expectedStats, stats);
	std::array<TypeParam::value_type, c> expected;
	
	// update via at
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<TypeParam::value_type>(i);
		myvec.at(i) = expected[i];
	}
	
	auto data = myvec.data();
	ASSERT_EQ(true, std::equal(data, data + c, begin(expected)));

	// test read on const vector
	const auto& myvec_c = myvec;
	for(std::size_t i=0; i<c; ++i)
	{
		ASSERT_EQ(expected[i], myvec_c.at(i));
	}

	// check allocator calls (shouldn't be any)
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, subscript_operator)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	myvec.resize(c);
	expectedStats.ConstructCalls += c;
	ASSERT_EQ(expectedStats, stats);
	std::array<TypeParam::value_type, c> expected;

	// update via []
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<TypeParam::value_type>(i);
		myvec[i] = expected[i];
	}

	auto data = myvec.data();
	ASSERT_EQ(true, std::equal(data, data + c, begin(expected)));

	// test read on const vectors
	const auto& myvec_c = myvec;
	for(std::size_t i=0; i<c; ++i)
	{
		ASSERT_EQ(expected[i], myvec_c[i]);	
	}
	
	// check allocator calls (shouldn't be any)
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, pop_back)
{
	typedef AllocatorMock<TypeParam::value_type> alloc_t;
	typedef alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	std::array<TypeParam::value_type, c> expected;

	// fill vector
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<TypeParam::value_type>(i);
		myvec.push_back(expected[i]);
	}
	expectedStats.ConstructCalls += c;
	ASSERT_EQ(expectedStats, stats);

	// pop empty
	for(std::size_t i=0; i<c; ++i)
	{
		myvec.pop_back();
		std::size_t expectedSize = c-i-1;
		ASSERT_EQ(expectedSize, myvec.size());
		const auto data = myvec.data();
		ASSERT_EQ(true, std::equal(data, data + expectedSize, begin(expected)));
		// check allocator calls
		if(!std::is_scalar<TypeParam::value_type>::value)
			++expectedStats.DestroyCalls;
		ASSERT_EQ(expectedStats, stats);
	}
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	int result = RUN_ALL_TESTS();
	system("PAUSE");
	return result;
}


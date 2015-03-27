
#include "vector.h"
#include "allocator_mock.h"
#include <array>
#include <gtest/gtest.h>

template<typename T>
typename std::enable_if<!std::is_pointer<T>::value, T>::type construct(int id = 0) { return T(id); }
template<typename T>
typename std::enable_if<std::is_pointer<T>::value, T>::type construct(int id = 0) { return reinterpret_cast<T>(id); }
template<>
std::string construct<std::string>(int id) { return std::string(id, '~'); }
template<>
std::pair<short, short> construct<std::pair<short, short>>(int id) { return std::make_pair(id, id); }


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
> TypesUnderTest;
TYPED_TEST_CASE(fcv_basic_test, TypesUnderTest);


TYPED_TEST(fcv_basic_test, ctor_and_capacity)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	ASSERT_EQ(0, myvec.size());

	std::array<typename TypeParam::value_type, c> expected;

	// test valid sizes (shrinking and growing)
	auto data = myvec.data();
	for(auto s : { 3, 7, 1, 8, 0, 4 })
	{
		// store current values in expected
		auto oldSize = myvec.size();
		for(std::size_t i=0; i<oldSize; ++i)
			expected[i] = data[i];
	
		// resize and fill with new size as value
		ASSERT_NO_THROW(myvec.resize(s, construct<typename TypeParam::value_type>(s)));
		
		// check correct size
		auto newSize = myvec.size();
		ASSERT_EQ(s, newSize);

		// check allocator calls
		if(oldSize < newSize)
			expectedStats.ConstructCalls += (newSize - oldSize);
		else if(oldSize > newSize)
		{
			expectedStats.DestroyCalls += 
				std::is_scalar<typename TypeParam::value_type>::value ? 0 : (oldSize - newSize);
		}
		ASSERT_EQ(expectedStats, stats);

		// check unchanged elements
		ASSERT_EQ(true, std::equal(data, data + std::min(oldSize, newSize), begin(expected)));
		// check new elements
 		for(std::size_t i=oldSize; i<newSize; ++i)
 			ASSERT_EQ(construct<typename TypeParam::value_type>(s), data[i]);
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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));

	std::array<typename TypeParam::value_type, c> expected;

	// cases where vector is not full
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<typename TypeParam::value_type>(i);

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
	const auto value = construct<typename TypeParam::value_type>(c);
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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));

	std::array<typename TypeParam::value_type, c> expected;

	// cases where vector is not full
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<typename TypeParam::value_type>(i);

		ASSERT_NO_THROW(myvec.push_back(construct<typename TypeParam::value_type>(i)));
		ASSERT_EQ(i+1, myvec.size());

		// check for allocator mock calls
		++expectedStats.ConstructCalls;
		ASSERT_EQ(expectedStats, stats);

		// check for expected content
		auto data = myvec.data();
		ASSERT_EQ(true, std::equal(data, data + i + 1, begin(expected)));
	}

	// cases where vector is full
	ASSERT_THROW(myvec.push_back(construct<typename TypeParam::value_type>(c)), std::length_error);
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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

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
		std::is_scalar<typename TypeParam::value_type>::value ? 0 : size;
	++expectedStats.DeallocateCalls;
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, clear)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	myvec.resize(c/2);
	expectedStats.ConstructCalls += (c/2);
	ASSERT_EQ(expectedStats, stats);

	auto size = myvec.size();
	myvec.clear();
	expectedStats.DestroyCalls +=
		std::is_scalar<typename TypeParam::value_type>::value ? 0 : size;
	ASSERT_EQ(expectedStats, stats);
}

TYPED_TEST(fcv_basic_test, copy_constructor)
{	
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	for(std::size_t c : { 0, 8 })
	{
		stats_t stats, expectedStats(c ? 1 : 0, 0, 0, 0);
		{
			TypeParam myvec(c, alloc_t(&stats));
			// fill src vector
			for(std::size_t i=0; i<c; ++i)
				myvec.push_back(construct<typename TypeParam::value_type>(i));
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

		expectedStats.DestroyCalls = std::is_scalar<typename TypeParam::value_type>::value 
			? 0 : expectedStats.ConstructCalls;
		expectedStats.DeallocateCalls = c ? 2 : 0;
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, move_constructor)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	for(std::size_t c : { 0, 8 })
	{
		stats_t stats, expectedStats(c ? 1 : 0, 0, 0, 0);
		{
			TypeParam myvec(c, alloc_t(&stats));
			// fill src vector
			for(std::size_t i=0; i<c; ++i)
				myvec.push_back(construct<typename TypeParam::value_type>(i));
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
				ASSERT_EQ(construct<typename TypeParam::value_type>(i), data2[i]);

			// check allocator calls
			ASSERT_EQ(expectedStats, stats);
		}

		expectedStats.DestroyCalls = std::is_scalar<typename TypeParam::value_type>::value 
			? 0 : expectedStats.ConstructCalls;
		expectedStats.DeallocateCalls = c ? 1 : 0;
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, initlist_constructor)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	std::initializer_list<typename TypeParam::value_type> il{
		construct<typename TypeParam::value_type>(0),
		construct<typename TypeParam::value_type>(1),
		construct<typename TypeParam::value_type>(2),
		construct<typename TypeParam::value_type>(3)
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
				std::is_scalar<typename TypeParam::value_type>::value ? 0 : myvec.size();
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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));

	for(std::size_t i=0; i<(c/2); ++i)
		myvec.push_back(construct<typename TypeParam::value_type>(i));
	expectedStats.ConstructCalls += (c/2);

	// assign vector of same capacity (smaller = larger)
	{
		TypeParam myvec2(c, alloc_t(&stats));
		myvec2.push_back(construct<typename TypeParam::value_type>());
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
			std::is_scalar<typename TypeParam::value_type>::value ? 0 : myvec2.size();
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
		expectedStats.DestroyCalls += std::is_scalar<typename TypeParam::value_type>::value 
			? 0 : (oldSize - myvec2.size());
		ASSERT_EQ(expectedStats, stats);
		
		// track stats for destruction of myvec2
		expectedStats.DestroyCalls += std::is_scalar<typename TypeParam::value_type>::value 
			? 0 : myvec2.size();
		++expectedStats.DeallocateCalls;
	}

	// assign vector of different capacity
	{
		TypeParam myvec2(c + 3, alloc_t(&stats));
		myvec2.push_back(construct<typename TypeParam::value_type>());
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
			std::is_scalar<typename TypeParam::value_type>::value ? 0 : oldSize;
		++expectedStats.DeallocateCalls;
		++expectedStats.AllocateCalls;
		expectedStats.ConstructCalls += myvec2.size();
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, move_assign)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(0, 0, 0, 0);
	TypeParam myvec(0, alloc_t(&stats));
	std::array<typename TypeParam::value_type, 8> expected;
	for(std::size_t i=0; i<8; ++i)
		expected[i] = construct<typename TypeParam::value_type>(i);

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
			std::is_scalar<typename TypeParam::value_type>::value ? 0 : oldSize;
		expectedStats.DeallocateCalls += (oldCapacity ? 1 : 0);
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, initlist_assign)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	std::initializer_list<typename TypeParam::value_type> empty_il {};
	std::initializer_list<typename TypeParam::value_type> small_il { construct<typename TypeParam::value_type>(0),  
		construct<typename TypeParam::value_type>(1), construct<typename TypeParam::value_type>(2) };
	std::initializer_list<typename TypeParam::value_type> medium_il { construct<typename TypeParam::value_type>(0),  
		construct<typename TypeParam::value_type>(1), construct<typename TypeParam::value_type>(2),  
		construct<typename TypeParam::value_type>(3), construct<typename TypeParam::value_type>(4) };
	std::initializer_list<typename TypeParam::value_type> large_il { construct<typename TypeParam::value_type>(0),  
		construct<typename TypeParam::value_type>(1), construct<typename TypeParam::value_type>(2),  
		construct<typename TypeParam::value_type>(3), construct<typename TypeParam::value_type>(4),  
		construct<typename TypeParam::value_type>(5), construct<typename TypeParam::value_type>(6) };
	
	std::array<typename TypeParam::value_type, 8> expected;

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
						std::is_scalar<typename TypeParam::value_type>::value ? 0 : (oldSize - il.size());
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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));	
	myvec.resize(c);
	expectedStats.ConstructCalls += c;
	ASSERT_EQ(expectedStats, stats);
	std::array<typename TypeParam::value_type, c> expected;
	
	// update via at
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<typename TypeParam::value_type>(i);
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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	myvec.resize(c);
	expectedStats.ConstructCalls += c;
	ASSERT_EQ(expectedStats, stats);
	std::array<typename TypeParam::value_type, c> expected;

	// update via []
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<typename TypeParam::value_type>(i);
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
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	std::array<typename TypeParam::value_type, c> expected;

	// fill vector
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<typename TypeParam::value_type>(i);
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
		if(!std::is_scalar<typename TypeParam::value_type>::value)
			++expectedStats.DestroyCalls;
		ASSERT_EQ(expectedStats, stats);
	}
}

TYPED_TEST(fcv_basic_test, iterators)
{
	std::array<typename TypeParam::value_type, 8> expected;
	for(std::size_t i=0; i<8; ++i)
		expected[i] = construct<typename TypeParam::value_type>(i);

	for(std::size_t c : { 0, 8 })
	{
		TypeParam myvec(c);
		for(std::size_t s : { 0, 4, 8 })
		{
			if(s <= c)
			{
				myvec.resize(s);
				for(std::size_t i=0; i<s; ++i)
					myvec[i] = expected[i];

				{ // begin & end
					auto b = myvec.begin();
					auto e = myvec.end();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=0; b != e; ++b, ++i)
						ASSERT_EQ(expected[i], *b);
				}
				{ // cbegin & cend					
					auto b = myvec.cbegin();
					auto e = myvec.cend();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=0; b != e; ++b, ++i)
						ASSERT_EQ(expected[i], *b);
				}
				{ // begin & end on const vector
					const auto& c_myvec = myvec;
					auto b = c_myvec.begin();
					auto e = c_myvec.end();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=0; b != e; ++b, ++i)
						ASSERT_EQ(expected[i], *b);
				}
				{ // cbegin & cend on const vector
					const auto& c_myvec = myvec;
					auto b = c_myvec.cbegin();
					auto e = c_myvec.cend();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=0; b != e; ++b, ++i)
						ASSERT_EQ(expected[i], *b);
				}
				{ // rbegin & rend
					auto b = myvec.rbegin();
					auto e = myvec.rend();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=s-1; b != e; ++b, --i)
						ASSERT_EQ(expected[i], *b);				
				}
				{ // crbegin & crend
					auto b = myvec.crbegin();
					auto e = myvec.crend();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=s-1; b != e; ++b, --i)
						ASSERT_EQ(expected[i], *b);				
				}
				{ // rbegin & rend on const vector
					const auto& c_myvec = myvec;
					auto b = c_myvec.rbegin();
					auto e = c_myvec.rend();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=s-1; b != e; ++b, --i)
						ASSERT_EQ(expected[i], *b);				
				}
				{ // crbegin & crend on const vector
					const auto& c_myvec = myvec;
					auto b = c_myvec.crbegin();
					auto e = c_myvec.crend();
					ASSERT_EQ(s, e - b);
					for(std::size_t i=s-1; b != e; ++b, --i)
						ASSERT_EQ(expected[i], *b);				
				}
			}
		}	
	}
}

TYPED_TEST(fcv_basic_test, swap)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	stats_t stats, expectedStats(1, 0, 0, 0);
	const std::size_t c = 8;
	TypeParam myvec(c, alloc_t(&stats));
	
	for(std::size_t c2 : { 4, 8, 0, 2 })
	{
		TypeParam myvec2(c2, alloc_t(&stats));
		myvec2.resize(c2 / 2, construct<typename TypeParam::value_type>(c2));
		expectedStats.AllocateCalls += (c2 ? 1 : 0);
		expectedStats.ConstructCalls += (c2 / 2);
		ASSERT_EQ(expectedStats, stats);

		auto oldCapacity = myvec.capacity();
		auto oldSize = myvec.size();
		auto oldBuffer = myvec.data();
		auto newBuffer = myvec2.data();
		myvec.swap(myvec2);
		// check capacity, size and buffer
		ASSERT_EQ(c2, myvec.capacity());
		ASSERT_EQ(oldCapacity, myvec2.capacity());
		ASSERT_EQ(c2 / 2, myvec.size());
		ASSERT_EQ(oldSize, myvec2.size());
		ASSERT_EQ(newBuffer, myvec.data());
		ASSERT_EQ(oldBuffer, myvec2.data());
		// check allocator mock
		ASSERT_EQ(expectedStats, stats);
	
		// track destruction 
		expectedStats.DestroyCalls +=
			std::is_scalar<typename TypeParam::value_type>::value ? 0 : myvec2.size();
		expectedStats.DeallocateCalls += (myvec2.capacity() ? 1 : 0);
	}
}

TYPED_TEST(fcv_basic_test, front_and_back)
{
	std::array<typename TypeParam::value_type, 8> expected;
	const std::size_t s = 4;
	for(std::size_t i=0; i<s; ++i)
		expected[i] = construct<typename TypeParam::value_type>(i);

	for(std::size_t c : { 4, 8 })
	{
		TypeParam myvec(c);
		for(std::size_t i=0; i<s; ++i)
			myvec.push_back(expected[i]);

		{	// test front
			ASSERT_EQ(expected[0], myvec.front());
		}	
		{	// test front on const vector
			const auto& c_myvec = myvec;
			ASSERT_EQ(expected[0], c_myvec.front());
		}
		{	// test back
			ASSERT_EQ(expected[myvec.size() - 1], myvec.back());
		}
		{	// test back on const vector
			const auto& c_myvec = myvec;
			ASSERT_EQ(expected[c_myvec.size() - 1], c_myvec.back());
		}
	}
}

TYPED_TEST(fcv_basic_test, insert_copy)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;
	
	// tests on non full vector
	{
		const std::size_t c = 8;
		const std::size_t s = 6;
		std::array<typename TypeParam::value_type, s> expected;
		for(std::size_t i=0; i<s; ++i)
			expected[i] = construct<typename TypeParam::value_type>(i);

		for(std::size_t offset : { 0, 1, 3, 5, 6 })
		{
			stats_t stats, expectedStats(1, 0, 0, 0);
			TypeParam myvec(c, alloc_t(&stats));
			for(std::size_t i=0; i<s; ++i)
				myvec.push_back(expected[i]);
			expectedStats.ConstructCalls += s;
			ASSERT_EQ(expectedStats, stats);

			auto pos = myvec.begin() + offset;
			const auto value = construct<typename TypeParam::value_type>(13);
			auto ret = myvec.insert(pos, value);
			// check capacity, size and content
			ASSERT_EQ(c, myvec.capacity());
			ASSERT_EQ(s + 1, myvec.size());
			for(std::size_t i=0; i<offset; ++i)
				ASSERT_EQ(expected[i], myvec[i]);
			ASSERT_EQ(value, myvec[offset]);
			for(std::size_t i=offset + 1; i<myvec.size(); ++i)
				ASSERT_EQ(expected[i-1], myvec[i]);
			// check return value
			ASSERT_EQ(offset, ret - myvec.begin());
			ASSERT_EQ(value, *ret);
			// check allocator mock
			++expectedStats.ConstructCalls;
			ASSERT_EQ(expectedStats, stats);	
		}
	}

	// test on full vector
	{
		std::array<typename TypeParam::value_type, 8> expected;
		for(std::size_t i=0; i<8; ++i)
			expected[i] = construct<typename TypeParam::value_type>(i);

		for(std::size_t c : { 0, 8 })
		{
			stats_t stats, expectedStats(c ? 1 : 0, 0, 0, 0);
			TypeParam myvec(c, alloc_t(&stats));
			for(std::size_t i=0; i<c; ++i)
				myvec.push_back(expected[i]);
			expectedStats.ConstructCalls += c;
			ASSERT_EQ(expectedStats, stats);

			const auto value = construct<typename TypeParam::value_type>(13);
			ASSERT_THROW(myvec.insert(myvec.begin(), value), std::length_error);

			// check that nothing has changed
			ASSERT_EQ(c, myvec.capacity());
			ASSERT_EQ(c, myvec.size());
			for(std::size_t i=0; i<myvec.size(); ++i)
				ASSERT_EQ(expected[i], myvec[i]);
			ASSERT_EQ(expectedStats, stats);
		}
	}
}

TYPED_TEST(fcv_basic_test, insert_move)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;
	
	// tests on non full vector
	{
		const std::size_t c = 8;
		const std::size_t s = 6;
		std::array<typename TypeParam::value_type, s> expected;
		for(std::size_t i=0; i<s; ++i)
			expected[i] = construct<typename TypeParam::value_type>(i);

		for(std::size_t offset : { 0, 1, 3, 5, 6 })
		{
			stats_t stats, expectedStats(1, 0, 0, 0);
			TypeParam myvec(c, alloc_t(&stats));
			for(std::size_t i=0; i<s; ++i)
				myvec.push_back(expected[i]);
			expectedStats.ConstructCalls += s;
			ASSERT_EQ(expectedStats, stats);

			auto pos = myvec.begin() + offset;
			auto ret = myvec.insert(pos, construct<typename TypeParam::value_type>(13));
			// check capacity, size and content
			ASSERT_EQ(c, myvec.capacity());
			ASSERT_EQ(s + 1, myvec.size());
			for(std::size_t i=0; i<offset; ++i)
				ASSERT_EQ(expected[i], myvec[i]);
			ASSERT_EQ(construct<typename TypeParam::value_type>(13), myvec[offset]);
			for(std::size_t i=offset + 1; i<myvec.size(); ++i)
				ASSERT_EQ(expected[i-1], myvec[i]);
			// check return value
			ASSERT_EQ(offset, ret - myvec.begin());
			ASSERT_EQ(construct<typename TypeParam::value_type>(13), *ret);
			// check allocator mock
			++expectedStats.ConstructCalls;
			ASSERT_EQ(expectedStats, stats);	
		}
	}

	// test on full vector
	{
		std::array<typename TypeParam::value_type, 8> expected;
		for(std::size_t i=0; i<8; ++i)
			expected[i] = construct<typename TypeParam::value_type>(i);

		for(std::size_t c : { 0, 8 })
		{
			stats_t stats, expectedStats(c ? 1 : 0, 0, 0, 0);
			TypeParam myvec(c, alloc_t(&stats));
			for(std::size_t i=0; i<c; ++i)
				myvec.push_back(expected[i]);
			expectedStats.ConstructCalls += c;
			ASSERT_EQ(expectedStats, stats);

			ASSERT_THROW(myvec.insert(myvec.begin(), 
				construct<typename TypeParam::value_type>(13)), std::length_error);

			// check that nothing has changed
			ASSERT_EQ(c, myvec.capacity());
			ASSERT_EQ(c, myvec.size());
			for(std::size_t i=0; i<myvec.size(); ++i)
				ASSERT_EQ(expected[i], myvec[i]);
			ASSERT_EQ(expectedStats, stats);
		}
	}
}

TYPED_TEST(fcv_basic_test, erase)
{
	typedef AllocatorMock<typename TypeParam::value_type> alloc_t;
	typedef typename alloc_t::Statistics stats_t;

	const std::size_t c = 8;
	std::array<typename TypeParam::value_type, c> expected;
	for(std::size_t i=0; i<c; ++i)
		expected[i] = construct<typename TypeParam::value_type>(i);

	for(std::size_t s : { 1, 4, 8 })
	{
		for(std::size_t offset : { 0, 1, 3, 6, 7 })
		{
			if(offset < s)
			{
				stats_t stats, expectedStats(1, 0, 0, 0);
				TypeParam myvec(c, alloc_t(&stats));
				for(std::size_t i=0; i<s; ++i)
					myvec.push_back(expected[i]);
				expectedStats.ConstructCalls += s;
				ASSERT_EQ(expectedStats, stats);

				auto ret = myvec.erase(myvec.begin() + offset);

				// check capacity, size and content
				ASSERT_EQ(c, myvec.capacity());
				ASSERT_EQ(s-1, myvec.size());
				for(std::size_t i=0; i<offset; ++i)
					ASSERT_EQ(expected[i], myvec[i]);
				for(std::size_t i=offset; i<myvec.size(); ++i)
					ASSERT_EQ(expected[i+1], myvec[i]);
				// check return value
				ASSERT_EQ(myvec.begin() + offset, ret);
				
				// check allocator mock
				expectedStats.DestroyCalls += 
					std::is_scalar<typename TypeParam::value_type>::value ? 0 : 1;
				ASSERT_EQ(expectedStats, stats);			
			}		
		}	
	}
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	int result = RUN_ALL_TESTS();
	return result;
}


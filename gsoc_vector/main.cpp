
#include "vector.h"
#include <array>
#include <gtest/gtest.h>


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
T construct(int id = 0) { return T(); }
template<>
int construct<int>(int id) { return id; }
template<>
std::string construct<std::string>(int id) { return std::string(id, '~'); }
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
	fixed_capacity_vector<int>, 
// 	fixed_capacity_vector<void*>,
// 	fixed_capacity_vector<const void*>,
	fixed_capacity_vector<std::string>
// 	fixed_capacity_vector<std::pair<float, double>>,
// 	fixed_capacity_vector<NonDefaultConstructable>,
// 	fixed_capacity_vector<NonCopyable>
> TypesUnderTest;
TYPED_TEST_CASE(fcv_basic_test, TypesUnderTest);


TYPED_TEST(fcv_basic_test, capacity)
{
	for(auto c : { 0, 1, 2, 3, 7, 19, 23, 100, 1000, 1234, 1337, 8000 })
	{
		const TypeParam myvec(c);
		ASSERT_EQ(c, myvec.capacity());
	}
}

TYPED_TEST(fcv_basic_test, size)
{
	// testing size is not really possible without using functions like push_back and resize
	// so we only test for size() = 0 on empty vector and for size() beeing callable on const objects
	const std::size_t c = 8;
	const TypeParam myvec(c);

	ASSERT_EQ(0, myvec.size());
}

TYPED_TEST(fcv_basic_test, resize)
{
	const std::size_t c = 8;
	TypeParam myvec(c);
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

		// check unchanged elements
		ASSERT_EQ(true, std::equal(data, data + std::min(oldSize, newSize), begin(expected)));
		// check new elements
 		for(std::size_t i=oldSize; i<newSize; ++i)
 			ASSERT_EQ(construct<TypeParam::value_type>(s), data[i]);
	}
}

TYPED_TEST(fcv_basic_test, push_back_copy)
{
	const std::size_t c = 8;
	TypeParam myvec(c);

	std::array<TypeParam::value_type, c> expected;

	// cases where vector is not full
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<TypeParam::value_type>(i);

		ASSERT_NO_THROW(myvec.push_back(expected[i]));
		ASSERT_EQ(i+1, myvec.size());

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
}

TYPED_TEST(fcv_basic_test, push_back_move)
{
	const std::size_t c = 8;
	TypeParam myvec(c);

	std::array<TypeParam::value_type, c> expected;

	// cases where vector is not full
	for(std::size_t i=0; i<c; ++i)
	{
		expected[i] = construct<TypeParam::value_type>(i);

		ASSERT_NO_THROW(myvec.push_back(construct<TypeParam::value_type>(i)));
		ASSERT_EQ(i+1, myvec.size());

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
}

TYPED_TEST(fcv_basic_test, at)
{
	const std::size_t c = 8;
	TypeParam myvec(c);	
	myvec.resize(c);
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
}

TYPED_TEST(fcv_basic_test, subscript_operator)
{
	const std::size_t c = 8;
	TypeParam myvec(c);
	myvec.resize(c);
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
}


int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	int result = RUN_ALL_TESTS();
	system("PAUSE");
	return result;
}


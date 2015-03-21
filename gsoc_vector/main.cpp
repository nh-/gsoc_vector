
#include "vector.h"
#include <gtest/gtest.h>

template<typename T>
class fcv_test : public ::testing::Test
{
public:
};

struct NonDefaultConstructable
{
	NonDefaultConstructable(int x)
		: x_(x)
	{
	}

	int x_;
};

typedef ::testing::Types<
	fixed_capacity_vector<int>, 
	fixed_capacity_vector<std::string>,
	fixed_capacity_vector<NonDefaultConstructable>
> TypesUnderTest;
TYPED_TEST_CASE(fcv_test, TypesUnderTest);

TYPED_TEST(fcv_test, capacity)
{
	for(auto s : { 1, 2, 3, 7, 19, 23, 100, 1000, 1234, 1337, 8000 })
	{
		TypeParam myvec(s);
		EXPECT_EQ(s, myvec.capacity());
	}
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	int result = RUN_ALL_TESTS();
	system("PAUSE");
	return result;
}


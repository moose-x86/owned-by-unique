/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Przemyslaw Wos
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
**/
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ptr_owned_by_unique.hpp"
#include "gmock_macros_for_unique_ptr.hpp"

using namespace ::pobu;
using namespace ::testing;

class owned_by_unique_test_suite : public ::testing::Test
{
protected:
  struct simple_base_class
  {
    virtual ~simple_base_class() = default;
  };

  struct destruction_test_mock : public simple_base_class
  {
    int x;

    destruction_test_mock(int y = 1) : x(y) {}

    MOCK_METHOD0(die, void());
    virtual ~destruction_test_mock(){ x = 0; die(); }
  };

  struct mock_interface
  {
    virtual int test(const std::unique_ptr<simple_base_class>) const = 0;
    virtual ~mock_interface() = default;
  };

  struct mock_class : public mock_interface
  {
    MOCK_UNIQUE_CONST_METHOD1(test, int(const std::unique_ptr<simple_base_class>));
  };

  typedef StrictMock<destruction_test_mock> test_mock;

  template<typename T> void assert_that_operators_throw(const ptr_owned_by_unique<T> &p)
  {
    ASSERT_THROW(*p, ptr_is_already_deleted);
    ASSERT_THROW(p.get(), ptr_is_already_deleted);
    ASSERT_THROW(p.operator->(), ptr_is_already_deleted);
  }

  template<typename T> void assert_that_operators_dont_throw(const ptr_owned_by_unique<T> &p)
  {
    ASSERT_NO_THROW(*p);
    ASSERT_NO_THROW(p.get());
    ASSERT_NO_THROW(p.operator->());
  }

  template<typename T> void assert_that_get_unique_throws(const ptr_owned_by_unique<T> &p)
  {
    ASSERT_THROW(p.unique_ptr(), unique_ptr_already_acquired);
  }

  template<typename T>
  std::unique_ptr<T> expect_that_get_unique_dont_throw(const ptr_owned_by_unique<T> &p)
  {
    std::unique_ptr<T> u;
    EXPECT_NO_THROW(u = p.unique_ptr());

    return u;
  }

  template<typename T> bool equal(const ptr_owned_by_unique<T> &p1, const ptr_owned_by_unique<T> &p2)
  {
    return (p1.get() == p2.get()) and (p1.is_acquired() == p2.is_acquired());
  }

  void expect_object_will_be_deleted(const ptr_owned_by_unique<destruction_test_mock> p)
  {
    EXPECT_CALL(*p, die()).Times(1);
  }

  void create_nine_copies_of(const ptr_owned_by_unique<destruction_test_mock> &p)
  {
    static std::vector<ptr_owned_by_unique<destruction_test_mock>> copiesOfPointers;
    copiesOfPointers = {p, p, p, p, p, p, p, p, p};
  }

  template<typename T>
  void release_unique_ptr_and_delete_object(std::unique_ptr<T> &u)
  {
    delete u.release();
  }

  void test_link_semantics(ptr_owned_by_unique<simple_base_class> p)
  {
    ASSERT_TRUE(p.is_acquired());
  }

  void test_move_semantics(ptr_owned_by_unique<test_mock> p)
  {
    ASSERT_FALSE(p.is_acquired());
  }
};

TEST_F(owned_by_unique_test_suite, isUniqueAndPtrOwnedPointingSameAddress)
{
  auto p = make_owned_by_unique<int>();
  auto u = expect_that_get_unique_dont_throw(p);

  ASSERT_EQ(u.get(), p.get());
}

TEST_F(owned_by_unique_test_suite, testCreatingPtrOwnedByUniqueFromNullptr)
{
  ptr_owned_by_unique<int> p = nullptr;
  auto u = expect_that_get_unique_dont_throw(p);

  ASSERT_EQ(u.get(), p.get());
  ASSERT_TRUE(not u);
  ASSERT_TRUE(not p);
}

TEST_F(owned_by_unique_test_suite, copyConstructorTest)
{
  auto p1 = make_owned_by_unique<int>();
  auto p2 = p1;

  ASSERT_TRUE(equal(p1, p2));

  auto u = expect_that_get_unique_dont_throw(p2);

  ASSERT_TRUE(equal(p1, p2));

  assert_that_get_unique_throws(p1);
  assert_that_get_unique_throws(p2);
}

TEST_F(owned_by_unique_test_suite, testMoveAndLinkSemantics)
{
  auto p = make_owned_by_unique<test_mock>();
  expect_object_will_be_deleted(p);

  auto u = p.unique_ptr();

  test_link_semantics(link<simple_base_class>(u));
  test_link_semantics(link(u));

  const ptr_owned_by_unique<destruction_test_mock> r = link(u);
  ASSERT_TRUE(r.is_acquired());

  assert_that_operators_dont_throw(p);
  assert_that_operators_dont_throw(r);

  test_move_semantics(std::move(u));

  assert_that_operators_dont_throw(p);
  assert_that_operators_dont_throw(r);
}

TEST_F(owned_by_unique_test_suite, deleteAfterCopyDontInvalidateCopy)
{
  ptr_owned_by_unique<destruction_test_mock> copy;

  {
    auto p = make_owned_by_unique<test_mock>();
    copy = p;
  }

  Mock::VerifyAndClearExpectations(copy.get());

  assert_that_operators_dont_throw(copy);
  expect_object_will_be_deleted(copy);
}

TEST_F(owned_by_unique_test_suite, isAcquireByUniquePtr)
{
  auto p = make_owned_by_unique<int>();
  auto u = p.unique_ptr();

  ASSERT_TRUE(p.is_acquired());

  assert_that_get_unique_throws(p);
  assert_that_operators_dont_throw(p);
}

TEST_F(owned_by_unique_test_suite, objectWillBeDeleted)
{
  auto p = make_owned_by_unique<test_mock>(199);
  expect_object_will_be_deleted(p);
}

TEST_F(owned_by_unique_test_suite, objectWillBeDeletedOnceWhenUniqueIsAcquired)
{
  auto p = make_owned_by_unique<test_mock>();
  auto u = p.unique_ptr();

  expect_object_will_be_deleted(p);
}

TEST_F(owned_by_unique_test_suite, objectWillBeDeletedOnceWhenUniqueIsAcquiredAndReleased)
{
  auto p = make_owned_by_unique<test_mock>();
  auto u = p.unique_ptr();

  expect_object_will_be_deleted(p);
  release_unique_ptr_and_delete_object(u);

  assert_that_operators_throw(p);
}

TEST_F(owned_by_unique_test_suite, objectWillBeDeletedWhenMultipleSharedObjects)
{
  auto p = make_owned_by_unique<test_mock>();

  create_nine_copies_of(p);
  expect_object_will_be_deleted(p);

  ASSERT_FALSE(p.is_acquired());
  EXPECT_EQ(p.use_count(), 10u);
}

TEST_F(owned_by_unique_test_suite, forNullPointerInvokeUniquePtrHowManyYouWant)
{
  ptr_owned_by_unique<destruction_test_mock> p;

  for(int i = 0; i < 100; i++)
  {
    ASSERT_FALSE(p.unique_ptr());
    expect_that_get_unique_dont_throw(p);
    assert_that_operators_dont_throw(p);
  }
}

TEST_F(owned_by_unique_test_suite, runtimeErrorIsThrownWhenResourceDeleted)
{
  const auto p = make_owned_by_unique<test_mock>();
  create_nine_copies_of(p);

  const auto r = p;

  expect_object_will_be_deleted(p);
  {
    auto u = expect_that_get_unique_dont_throw(p);
  }

  assert_that_operators_throw(p);

  const auto w = p;

  assert_that_operators_throw(w);
  assert_that_operators_throw(r);
}

TEST_F(owned_by_unique_test_suite, noRuntimeErrorWhenResourceIsAquiredInUnique)
{
  const auto p = make_owned_by_unique<test_mock>(12324);
  const auto u = p.unique_ptr();

  expect_object_will_be_deleted(p);

  for(int i = 0; i < 100; i++)
    assert_that_operators_dont_throw(p);
}

TEST_F(owned_by_unique_test_suite, boolOperator)
{
  ptr_owned_by_unique<int> r;
  auto p = make_owned_by_unique<int>(12);

  ASSERT_FALSE(!p);
  ASSERT_FALSE(r);
}

TEST_F(owned_by_unique_test_suite, isUniquePtrValidAfterOwnedPtrDeletion)
{
  std::unique_ptr<test_mock> u;
  {
    auto p = make_owned_by_unique<test_mock>();

    p->x = 0x123;
    u = expect_that_get_unique_dont_throw(p);
  }

  Mock::VerifyAndClearExpectations(u.get());

  EXPECT_CALL(*u, die()).Times(1);
  ASSERT_EQ(u->x, 0x123);
}

TEST_F(owned_by_unique_test_suite, uniquePtrConstructor)
{
  std::unique_ptr<destruction_test_mock> u(new test_mock());
  ptr_owned_by_unique<destruction_test_mock> p(std::move(u));

  ASSERT_FALSE(u.get());
  expect_object_will_be_deleted(p);
}

TEST_F(owned_by_unique_test_suite, explicitOperatorTest)
{
  const auto p = make_owned_by_unique<int>();
  const std::unique_ptr<int> u(p);

  ASSERT_TRUE(u.get());
  assert_that_get_unique_throws(p);
}

TEST_F(owned_by_unique_test_suite, testConversionInGoogleMockParams)
{
  mock_class m;
  mock_interface& base = m;
  const auto p = make_owned_by_unique<test_mock>();

  expect_object_will_be_deleted(p);
  EXPECT_CALL(m, _test(Eq(p))).WillOnce(Return(0));

  base.test(p.unique_ptr());
  assert_that_operators_dont_throw(p);
}

TEST_F(owned_by_unique_test_suite, testIsNullAndNotNullMatchers)
{
  mock_class m;
  mock_interface& base = m;
  const auto p = make_owned_by_unique<test_mock>(0x123);

  EXPECT_CALL(m, _test(NotNull())).WillOnce(Return(0));
  expect_object_will_be_deleted(p);

  base.test(p.unique_ptr());
  assert_that_operators_dont_throw(p);

  EXPECT_EQ(p->x, 0x123);

  EXPECT_CALL(m, _test(IsNull())).WillOnce(Return(0));
  base.test(nullptr);
}

TEST_F(owned_by_unique_test_suite, assertThatCompareOperatorsDontThrow)
{
  auto p = pobu::make_owned_by_unique<test_mock>();
  auto r = pobu::make_owned_by_unique<int>();
  {
    expect_object_will_be_deleted(p);
    std::unique_ptr<test_mock> u{p};
  }

  assert_that_operators_throw(p);

  ASSERT_NO_THROW(p == p);
  ASSERT_NO_THROW(p != r);
  ASSERT_NO_THROW(p == nullptr);
  ASSERT_NO_THROW(p != nullptr);
  ASSERT_NO_THROW(p < r);
  ASSERT_NO_THROW(p > r);
  ASSERT_NO_THROW(p <= r);
  ASSERT_NO_THROW(p >= r);

  ASSERT_EQ(p, p);
  ASSERT_NE(p, r);
  ASSERT_TRUE(p < r);
  ASSERT_TRUE(p <= r);
  ASSERT_FALSE(p > r);
  ASSERT_FALSE(p >= r);
}

TEST_F(owned_by_unique_test_suite, assertThatSharedStateWillBeUpdateAfterPtrOwnedDeletion)
{
  std::unique_ptr<test_mock> u;
  {
    auto p = make_owned_by_unique<test_mock>();
    u = p.unique_ptr();
  }

  ptr_owned_by_unique<test_mock> p{std::move(u)};
  expect_object_will_be_deleted(p);

  p.unique_ptr().reset();
  assert_that_operators_throw(p);
}

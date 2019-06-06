#include <memory>
#include <string>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../src/variant.hpp"

using util::variant;
using util::get;
using util::bad_get;

namespace {
    struct helper{
        int tag;
        int * destructor_cnt;
        int * copy_cnt;
        bool throw_on_copy;
        helper(int tag, int * destructor_cnt, int * copy_cnt, bool throw_on_copy = false)
                : tag(tag)
                , destructor_cnt(destructor_cnt)
                , copy_cnt(copy_cnt)
                , throw_on_copy(throw_on_copy)
        {}

        helper(const helper& other)
            : tag(other.tag)
            , destructor_cnt(other.destructor_cnt)
            , copy_cnt(other.copy_cnt)
            , throw_on_copy(other.throw_on_copy)
        {
            (*copy_cnt)++;
            if (throw_on_copy)
                throw std::bad_alloc();
        }

        helper(helper&& other) = default;
        helper & operator=(helper const & other) = default;
        helper & operator=(helper && other) = default;

        virtual ~helper() { (*destructor_cnt)++; }
    };

    struct A {  };
    struct B : A {  };
}

TEST_CASE("Default constructor") {
    variant<int> a;
    CHECK_THROWS_AS(get<int>(a), bad_get);

    variant<int, double, short> b;
    CHECK_THROWS_AS(get<short>(b), bad_get);
}

TEST_CASE("Value constructor") {
    variant<int, double> a(5.0);
    CHECK(get<double>(a) == 5.0);
    CHECK_THROWS_AS(get<int>(a), bad_get);

    variant<int, double> b(5);
    CHECK(get<int>(b) == 5);
    CHECK_THROWS_AS(get<double>(b), bad_get);

    variant<int, std::string> c("Hello");
    CHECK(get<std::string>(c) == "Hello");
    CHECK_THROWS_AS(get<int>(c), bad_get);
}

TEST_CASE("Move semantics") {
    std::unique_ptr<int> ptr(new int);

    variant<int, std::unique_ptr<int>> a(std::move(ptr));
    variant<int, std::unique_ptr<int>> b;
    b = std::move(a);
    CHECK(!get<std::unique_ptr<int>>(a));
    CHECK(get<std::unique_ptr<int>>(b));

    std::unique_ptr<int> ptr2 = get<std::unique_ptr<int>>(std::move(b));
    CHECK(ptr2);
    CHECK(!get<std::unique_ptr<int>>(a));
    CHECK(!get<std::unique_ptr<int>>(b));
}

TEST_CASE("get") {
    const variant<int, helper> a(3);
    CHECK(get<int>(a) == 3);

    int i = 5;
    const variant<int*> b(&i);
    CHECK(*get<int*>(b) == 5);

    variant<int, helper> ax(3);
    CHECK(get<int>(ax) == 3);

    variant<int*> bx(&i);
    CHECK(*get<int*>(bx) == 5);
}

TEST_CASE("get by pointer") {
    variant<int, helper> a(3);
    CHECK(*get<int>(&a) == 3);
    CHECK(get<helper>(&a) == nullptr);

    variant<int, helper> const b(3);
    CHECK(*get<int>(&b) == 3);
    CHECK(get<helper>(&b) == nullptr);
}

TEST_CASE("get hiearchy") {
    variant<A, B> x;
    x = B();
    CHECK_THROWS_AS(get<A>(x), bad_get);
    CHECK_NOTHROW(get<B>(x));
}

TEST_CASE("get by index") {
    variant<int, std::string> a(3);
    CHECK(get<0>(a) == 3);
    const auto aint = a;
    a = "Hello";
    CHECK(get<1>(a) == "Hello");
    const auto astring = a;
    CHECK(get<0>(aint) == 3);
    CHECK(get<1>(astring) == "Hello");
}

TEST_CASE("get by index by pointer") {
    variant<int, std::string> a(3);
    CHECK(*get<0>(&a) == 3);
    CHECK(get<1>(&a) == nullptr);
    const auto aint = a;
    a = "Hello";
    CHECK(*get<1>(&a) == "Hello");
    CHECK(get<0>(&a) == nullptr);
    const auto astring = a;
    CHECK(*get<0>(&aint) == 3);
    CHECK(*get<1>(&astring) == "Hello");
}

TEST_CASE("Check of alignment") {
    struct alignas(128) X {};
    CHECK(alignof(variant<char, X>) == 128);
}

TEST_CASE("test swap") {
    using std::swap;
    variant<int, std::string> a(3);
    variant<int, std::string> b("Hello");
    CHECK(get<std::string>(b) == "Hello");
    CHECK(get<int>(a) == 3);
    swap(a, b);
    CHECK(get<std::string>(a) == "Hello");
    CHECK(get<int>(b) == 3);
}
//
TEST_CASE("Check destructors") {
    int destructor_count = 0;
    int copy_cnt = 0;
    helper *helper_ptr = new helper(5, &destructor_count, &copy_cnt);
    {
        variant<helper, int, double> a(helper(5, &destructor_count, &copy_cnt));
        variant<int, helper*, double> b(helper_ptr);
    }
    CHECK(destructor_count == 2);
    delete helper_ptr;
}

TEST_CASE("Test empty") {
    int destructor_count = 0;
    int copy_cnt = 0;
    variant<int, helper, double> b;
    CHECK(b.empty());
    b = helper(5, &destructor_count, &copy_cnt);
    CHECK(!b.empty());
}

TEST_CASE("Test clear") {
    int destructor_count = 0;
    int copy_cnt = 0;
    variant<int, helper, double> b(helper(5, &destructor_count, &copy_cnt));
    CHECK(destructor_count == 1); // Destroying temporary object
    CHECK(!b.empty());
    b.clear();
    CHECK(b.empty());
    CHECK(destructor_count == 2); // Destroying object on clear
}

TEST_CASE("Test index") {
    int destructor_count = 0;
    int copy_cnt = 0;
    variant<int, helper, std::string> b(helper(5, &destructor_count, &copy_cnt));
    CHECK(b.index() == 1);
    b = 5;
    CHECK(b.index() == 0);
    b = "Hello";
    CHECK(b.index() == 2);
}

TEST_CASE("Copy constructor") {
    int destructor_count = 0;
    int copy_cnt = 0;
    {
        variant<helper> a(helper(5, &destructor_count, &copy_cnt));
        variant<helper> b(a);

        CHECK(get<helper>(a).tag == 5);
        CHECK(get<helper>(b).tag == 5);
        CHECK(copy_cnt > 0);
    }
    CHECK(destructor_count == 3);
}

TEST_CASE("Copy operator=") {
    variant<int, helper> a(3);
    variant<int, helper> b(5);
    b = a;
    CHECK(get<int>(a) == 3);
    CHECK(get<int>(b) == 3);
    b = variant<int, helper>(6);
    CHECK(get<int>(b) == 6);
}

//#define CATCH_CONFIG_MAIN
//#include "catch.hpp"



//int f(int a, int b) {
//    return a + b;
//}
//
//TEST_CASE("Check sum") {
//    CHECK(f(1, 2) == 3);
//    CHECK(f(3, 2) == 4);
//}
//
//TEST_CASE("Check sum 2") {
//    CHECK(f(1, 2) == 3);
//    CHECK(f(3, 2) == 5);
//}

#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "../src/total_variant.hpp"

TEST_CASE() {
    total_variant v(5);
    CHECK(util::get<int>(v) == 5);
}

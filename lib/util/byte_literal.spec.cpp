//
// Created by ahodges on 26/05/22.
//

#include "byte_literal.hpp"

#include <doctest/doctest.h>

using namespace arby::byte_literals;

TEST_SUITE_BEGIN("byte_literal");

TEST_CASE("detail::pow")
{
        CHECK_EQ(detail::pow<0>(1),1);
        CHECK_EQ(detail::pow<1>(1),1);
        CHECK_EQ(detail::pow<2>(1),1);
        CHECK_EQ(detail::pow<3>(1),1);
        CHECK_EQ(detail::pow<0>(313),1);
        CHECK_EQ(detail::pow<5>(11),161051);
        CHECK_EQ(detail::pow<3>(37),50653);
        CHECK_EQ(detail::pow<6>(27),387420489);
}

TEST_CASE("literals")
{
    CHECK_EQ(1_Bit,1);
    CHECK_EQ(1000_Bit,1000);
    CHECK_EQ(30903181_Bit,30903181);

    CHECK_EQ(1_KB,1000);
    CHECK_EQ(10_KB,10000);
    CHECK_EQ(2345_KB,2345000);

    CHECK_EQ(1_KiB,1024);
    CHECK_EQ(10_KiB,10240);
    CHECK_EQ(2345_KiB,2401280);
}

TEST_SUITE_END();
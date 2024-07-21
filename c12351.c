TEST_CASE("Simple syntax test (with unicode)", "[general]")
{
    parser parser(
        u8" ROOT ← _ "
        " _ <- ' ' "
    );

    bool ret = parser;
    REQUIRE(ret == true);
}
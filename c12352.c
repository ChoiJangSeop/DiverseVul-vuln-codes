TEST_CASE("Simple syntax test", "[general]")
{
    parser parser(R"(
        ROOT <- _
        _ <- ' '
    )");

    bool ret = parser;
    REQUIRE(ret == true);
}
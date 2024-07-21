TEST_CASE("Backslash escape sequence test", "[general]")
{
    parser parser(R"(
        ROOT <- _
        _ <- '\\'
    )");

    bool ret = parser;
    REQUIRE(ret == true);
}
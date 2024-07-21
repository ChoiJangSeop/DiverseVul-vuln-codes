TEST_CASE("Empty syntax test", "[general]")
{
    parser parser("");
    bool ret = parser;
    REQUIRE(ret == false);
}
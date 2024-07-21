TEST_CASE("Negated Class test", "[general]")
{
    parser parser(R"(
        ROOT <- [^a-z_]+
    )");

    bool ret = parser;
    REQUIRE(ret == true);

    REQUIRE(parser.parse("ABC123"));
    REQUIRE_FALSE(parser.parse("ABcZ"));
    REQUIRE_FALSE(parser.parse("ABCZ_"));
    REQUIRE_FALSE(parser.parse(""));
}
TEST_CASE("Definition duplicates test", "[general]")
{
    parser parser(R"(
        A <- ''
        A <- ''
    )");

    REQUIRE(!parser);
}
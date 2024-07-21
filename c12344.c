TEST_CASE("Missing missing definitions test", "[general]")
{
    parser parser(R"(
        A <- B C
    )");

    REQUIRE(!parser);
}
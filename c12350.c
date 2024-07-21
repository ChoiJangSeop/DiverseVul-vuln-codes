TEST_CASE("Start rule with ignore operator test", "[general]")
{
    parser parser(R"(
        ~ROOT <- _
        _ <- ' '
    )");

    bool ret = parser;
    REQUIRE(ret == false);
}
TEST_CASE("Semantic value tag", "[general]")
{
    parser parser(R"(
        S <- A? B* C?
        A <- 'a'
        B <- 'b'
        C <- 'c'
    )");

    {
        using namespace udl;
        parser["S"] = [](const SemanticValues& sv) {
            REQUIRE(sv.size() == 1);
            REQUIRE(sv.tags.size() == 1);
            REQUIRE(sv.tags[0] == "C"_);
        };
        auto ret = parser.parse("c");
        REQUIRE(ret == true);
    }

    {
        using namespace udl;
        parser["S"] = [](const SemanticValues& sv) {
            REQUIRE(sv.size() == 2);
            REQUIRE(sv.tags.size() == 2);
            REQUIRE(sv.tags[0] == "B"_);
            REQUIRE(sv.tags[1] == "B"_);
        };
        auto ret = parser.parse("bb");
        REQUIRE(ret == true);
    }

    {
        using namespace udl;
        parser["S"] = [](const SemanticValues& sv) {
            REQUIRE(sv.size() == 2);
            REQUIRE(sv.tags.size() == 2);
            REQUIRE(sv.tags[0] == "A"_);
            REQUIRE(sv.tags[1] == "C"_);
        };
        auto ret = parser.parse("ac");
        REQUIRE(ret == true);
    }
}
TEST_CASE("Ordered choice count", "[general]")
{
    parser parser(R"(
        S <- 'a' / 'b'
    )");

    parser["S"] = [](const SemanticValues& sv) {
        REQUIRE(sv.choice() == 1);
        REQUIRE(sv.choice_count() == 2);
    };

    parser.parse("b");
}
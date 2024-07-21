TEST_CASE("Ordered choice count 2", "[general]")
{
    parser parser(R"(
        S <- ('a' / 'b')*
    )");

    parser["S"] = [](const SemanticValues& sv) {
        REQUIRE(sv.choice() == 0);
        REQUIRE(sv.choice_count() == 0);
    };

    parser.parse("b");
}
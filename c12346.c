TEST_CASE("Action taking non const Semantic Values parameter", "[general]")
{
    parser parser(R"(
        ROOT <- TEXT
        TEXT <- [a-zA-Z]+
    )");

    parser["ROOT"] = [&](SemanticValues& sv) {
        auto s = any_cast<std::string>(sv[0]);
        s[0] = 'H'; // mutate
        return std::string(std::move(s)); // move
    };

    parser["TEXT"] = [&](SemanticValues& sv) {
        return sv.token();
    };

    std::string val;
    auto ret = parser.parse("hello", val);
    REQUIRE(ret == true);
    REQUIRE(val == "Hello");
}
TEST_CASE("Semantic values test", "[general]")
{
    parser parser(R"(
        term <- ( a b c x )? a b c
        a <- 'a'
        b <- 'b'
        c <- 'c'
        x <- 'x'
    )");

	for (const auto& rule: parser.get_rule_names()){
		parser[rule.c_str()] = [rule](const SemanticValues& sv, any&) {
            if (rule == "term") {
                REQUIRE(any_cast<std::string>(sv[0]) == "a at 0");
                REQUIRE(any_cast<std::string>(sv[1]) == "b at 1");
                REQUIRE(any_cast<std::string>(sv[2]) == "c at 2");
                return std::string();
            } else {
                return rule + " at " + std::to_string(sv.c_str() - sv.ss);
            }
		};
	}

	REQUIRE(parser.parse("abc"));
}
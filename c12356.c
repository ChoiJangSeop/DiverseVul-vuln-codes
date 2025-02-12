TEST_CASE("String capture test", "[general]")
{
    parser parser(R"(
        ROOT      <-  _ ('[' TAG_NAME ']' _)*
        TAG_NAME  <-  (!']' .)+
        _         <-  [ \t]*
    )");

    std::vector<std::string> tags;

    parser["TAG_NAME"] = [&](const SemanticValues& sv) {
        tags.push_back(sv.str());
    };

    auto ret = parser.parse(" [tag1] [tag:2] [tag-3] ");

    REQUIRE(ret == true);
    REQUIRE(tags.size() == 3);
    REQUIRE(tags[0] == "tag1");
    REQUIRE(tags[1] == "tag:2");
    REQUIRE(tags[2] == "tag-3");
}
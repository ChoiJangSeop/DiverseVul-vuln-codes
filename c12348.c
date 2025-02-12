TEST_CASE("String capture test3", "[general]")
{
    parser pg(R"(
        ROOT  <- _ TOKEN*
        TOKEN <- '[' < (!']' .)+ > ']' _
        _     <- [ \t\r\n]*
    )");


    std::vector<std::string> tags;

    pg["TOKEN"] = [&](const SemanticValues& sv) {
        tags.push_back(sv.token());
    };

    auto ret = pg.parse(" [tag1] [tag:2] [tag-3] ");

    REQUIRE(ret == true);
    REQUIRE(tags.size() == 3);
    REQUIRE(tags[0] == "tag1");
    REQUIRE(tags[1] == "tag:2");
    REQUIRE(tags[2] == "tag-3");
}
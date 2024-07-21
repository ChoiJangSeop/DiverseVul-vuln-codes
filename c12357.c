TEST_CASE("Token check test", "[general]")
{
    parser parser(R"(
        EXPRESSION       <-  _ TERM (TERM_OPERATOR TERM)*
        TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
        FACTOR           <-  NUMBER / '(' _ EXPRESSION ')' _
        TERM_OPERATOR    <-  < [-+] > _
        FACTOR_OPERATOR  <-  < [/*] > _
        NUMBER           <-  < [0-9]+ > _
        _                <-  [ \t\r\n]*
    )");

    REQUIRE(parser["EXPRESSION"].is_token() == false);
    REQUIRE(parser["FACTOR"].is_token() == false);
    REQUIRE(parser["FACTOR_OPERATOR"].is_token() == true);
    REQUIRE(parser["NUMBER"].is_token() == true);
    REQUIRE(parser["_"].is_token() == true);
}
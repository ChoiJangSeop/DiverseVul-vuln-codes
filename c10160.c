TEST(RegexMatchExpression, RegexCannotBeInvalidUTF8) {
    ASSERT_THROWS_CODE(RegexMatchExpression("path", "^\xff\xff", ""), AssertionException, 5108300);
    ASSERT_THROWS_CODE(
        RegexMatchExpression("path", "^42", "\xff\xff"), AssertionException, 5108300);
}
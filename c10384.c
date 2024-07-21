TEST(QueryProjectionTest, InvalidElemMatchTextProjection) {
    assertInvalidProjection("{}", "{a: {$elemMatch: {$text: {$search: 'str'}}}}");
}
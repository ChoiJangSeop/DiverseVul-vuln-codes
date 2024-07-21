TEST(QueryProjectionTest, InvalidElemMatchExprProjection) {
    assertInvalidProjection("{}", "{a: {$elemMatch: {$expr: 5}}}");
}
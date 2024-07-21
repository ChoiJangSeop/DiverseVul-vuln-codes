TEST(QueryProjectionTest, InvalidPositionalOperatorProjections) {
    assertInvalidProjection("{}", "{'a.$': 1}");
    assertInvalidProjection("{a: 1}", "{'b.$': 1}");
    assertInvalidProjection("{a: 1}", "{'a.$': 0}");
    assertInvalidProjection("{a: 1}", "{'a.$.d.$': 1}");
    assertInvalidProjection("{a: 1}", "{'a.$.$': 1}");
    assertInvalidProjection("{a: 1, b: 1, c: 1}", "{'abc.$': 1}");
    assertInvalidProjection("{$or: [{a: 1}, {$or: [{b: 1}, {c: 1}]}]}", "{'d.$': 1}");
    assertInvalidProjection("{a: [1, 2, 3]}", "{'.$': 1}");
}
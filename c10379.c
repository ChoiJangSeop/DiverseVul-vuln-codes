TEST(QueryProjectionTest, InvalidElemMatchWhereProjection) {
    assertInvalidProjection("{}", "{a: {$elemMatch: {$where: 'this.a == this.b'}}}");
}
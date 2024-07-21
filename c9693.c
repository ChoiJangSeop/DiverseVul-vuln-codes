TEST(IndexBoundsBuilderTest, TranslateNotEqualToNullShouldBuildExactBoundsIfIndexIsNotMultiKey) {
    BSONObj indexPattern = BSON("a" << 1);
    auto testIndex = buildSimpleIndexEntry(indexPattern);

    BSONObj obj = BSON("a" << BSON("$ne" << BSONNULL));
    auto expr = parseMatchExpression(obj);

    OrderedIntervalList oil;
    IndexBoundsBuilder::BoundsTightness tightness;
    IndexBoundsBuilder::translate(
        expr.get(), indexPattern.firstElement(), testIndex, &oil, &tightness);

    // Bounds should be [MinKey, undefined), (null, MaxKey].
    ASSERT_EQUALS(oil.name, "a");
    ASSERT_EQUALS(tightness, IndexBoundsBuilder::EXACT);
    assertBoundsRepresentNotEqualsNull(oil);
}
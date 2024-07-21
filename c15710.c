TEST_F(DocumentSourceMatchTest, MultipleMatchStagesShouldCombineIntoOne) {
    auto match1 = DocumentSourceMatch::create(BSON("a" << 1), getExpCtx());
    auto match2 = DocumentSourceMatch::create(BSON("b" << 1), getExpCtx());
    auto match3 = DocumentSourceMatch::create(BSON("c" << 1), getExpCtx());

    Pipeline::SourceContainer container;

    // Check initial state
    ASSERT_BSONOBJ_EQ(match1->getQuery(), BSON("a" << 1));
    ASSERT_BSONOBJ_EQ(match2->getQuery(), BSON("b" << 1));
    ASSERT_BSONOBJ_EQ(match3->getQuery(), BSON("c" << 1));

    container.push_back(match1);
    container.push_back(match2);
    match1->optimizeAt(container.begin(), &container);

    ASSERT_EQUALS(container.size(), 1U);
    ASSERT_BSONOBJ_EQ(match1->getQuery(), fromjson("{'$and': [{a:1}, {b:1}]}"));

    container.push_back(match3);
    match1->optimizeAt(container.begin(), &container);
    ASSERT_EQUALS(container.size(), 1U);
    ASSERT_BSONOBJ_EQ(match1->getQuery(),
                      fromjson("{'$and': [{'$and': [{a:1}, {b:1}]},"
                               "{c:1}]}"));
}
TEST_F(ServerSelectorTestFixture, ShouldFilterByTags) {
    auto tags = TagSets::productionSet;
    auto servers = makeServerDescriptionList();
    selector.filterTags(&servers, tags);
    ASSERT_EQ(3, servers.size());

    tags = TagSets::eastOrWestProductionSet;
    servers = makeServerDescriptionList();
    selector.filterTags(&servers, tags);
    ASSERT_EQ(2, servers.size());

    tags = TagSets::testSet;
    servers = makeServerDescriptionList();
    selector.filterTags(&servers, tags);
    ASSERT_EQ(2, servers.size());

    tags = TagSets::integrationOrTestSet;
    servers = makeServerDescriptionList();
    selector.filterTags(&servers, tags);
    ASSERT_EQ(2, servers.size());

    tags = TagSets::westProductionSet;
    servers = makeServerDescriptionList();
    selector.filterTags(&servers, tags);
    ASSERT_EQ(1, servers.size());

    tags = TagSets::integrationSet;
    servers = makeServerDescriptionList();
    selector.filterTags(&servers, tags);
    ASSERT_EQ(0, servers.size());

    tags = TagSets::emptySet;
    servers = makeServerDescriptionList();
    selector.filterTags(&servers, tags);
    ASSERT_EQ(makeServerDescriptionList().size(), servers.size());
}
TEST(RoleParsingTest, BuildRoleBSON) {
    RoleGraph graph;
    RoleName roleA("roleA", "dbA");
    RoleName roleB("roleB", "dbB");
    RoleName roleC("roleC", "dbC");
    ActionSet actions;
    actions.addAction(ActionType::find);
    actions.addAction(ActionType::insert);

    ASSERT_OK(graph.createRole(roleA));
    ASSERT_OK(graph.createRole(roleB));
    ASSERT_OK(graph.createRole(roleC));

    ASSERT_OK(graph.addRoleToRole(roleA, roleC));
    ASSERT_OK(graph.addRoleToRole(roleA, roleB));
    ASSERT_OK(graph.addRoleToRole(roleB, roleC));

    ASSERT_OK(graph.addPrivilegeToRole(
        roleA, Privilege(ResourcePattern::forAnyNormalResource(), actions)));
    ASSERT_OK(graph.addPrivilegeToRole(
        roleB, Privilege(ResourcePattern::forExactNamespace(NamespaceString("dbB.foo")), actions)));
    ASSERT_OK(
        graph.addPrivilegeToRole(roleC, Privilege(ResourcePattern::forClusterResource(), actions)));
    ASSERT_OK(graph.recomputePrivilegeData());


    // Role A
    mutablebson::Document doc;
    ASSERT_OK(RoleGraph::getBSONForRole(&graph, roleA, doc.root()));
    BSONObj roleDoc = doc.getObject();

    ASSERT_EQUALS("dbA.roleA", roleDoc["_id"].String());
    ASSERT_EQUALS("roleA", roleDoc["role"].String());
    ASSERT_EQUALS("dbA", roleDoc["db"].String());

    std::vector<BSONElement> privs = roleDoc["privileges"].Array();
    ASSERT_EQUALS(1U, privs.size());
    ASSERT_EQUALS("", privs[0].Obj()["resource"].Obj()["db"].String());
    ASSERT_EQUALS("", privs[0].Obj()["resource"].Obj()["collection"].String());
    ASSERT(privs[0].Obj()["resource"].Obj()["cluster"].eoo());
    std::vector<BSONElement> actionElements = privs[0].Obj()["actions"].Array();
    ASSERT_EQUALS(2U, actionElements.size());
    ASSERT_EQUALS("find", actionElements[0].String());
    ASSERT_EQUALS("insert", actionElements[1].String());

    std::vector<BSONElement> roles = roleDoc["roles"].Array();
    ASSERT_EQUALS(2U, roles.size());
    ASSERT_EQUALS("roleC", roles[0].Obj()["role"].String());
    ASSERT_EQUALS("dbC", roles[0].Obj()["db"].String());
    ASSERT_EQUALS("roleB", roles[1].Obj()["role"].String());
    ASSERT_EQUALS("dbB", roles[1].Obj()["db"].String());

    // Role B
    doc.reset();
    ASSERT_OK(RoleGraph::getBSONForRole(&graph, roleB, doc.root()));
    roleDoc = doc.getObject();

    ASSERT_EQUALS("dbB.roleB", roleDoc["_id"].String());
    ASSERT_EQUALS("roleB", roleDoc["role"].String());
    ASSERT_EQUALS("dbB", roleDoc["db"].String());

    privs = roleDoc["privileges"].Array();
    ASSERT_EQUALS(1U, privs.size());
    ASSERT_EQUALS("dbB", privs[0].Obj()["resource"].Obj()["db"].String());
    ASSERT_EQUALS("foo", privs[0].Obj()["resource"].Obj()["collection"].String());
    ASSERT(privs[0].Obj()["resource"].Obj()["cluster"].eoo());
    actionElements = privs[0].Obj()["actions"].Array();
    ASSERT_EQUALS(2U, actionElements.size());
    ASSERT_EQUALS("find", actionElements[0].String());
    ASSERT_EQUALS("insert", actionElements[1].String());

    roles = roleDoc["roles"].Array();
    ASSERT_EQUALS(1U, roles.size());
    ASSERT_EQUALS("roleC", roles[0].Obj()["role"].String());
    ASSERT_EQUALS("dbC", roles[0].Obj()["db"].String());

    // Role C
    doc.reset();
    ASSERT_OK(RoleGraph::getBSONForRole(&graph, roleC, doc.root()));
    roleDoc = doc.getObject();

    ASSERT_EQUALS("dbC.roleC", roleDoc["_id"].String());
    ASSERT_EQUALS("roleC", roleDoc["role"].String());
    ASSERT_EQUALS("dbC", roleDoc["db"].String());

    privs = roleDoc["privileges"].Array();
    ASSERT_EQUALS(1U, privs.size());
    ASSERT(privs[0].Obj()["resource"].Obj()["cluster"].Bool());
    ASSERT(privs[0].Obj()["resource"].Obj()["db"].eoo());
    ASSERT(privs[0].Obj()["resource"].Obj()["collection"].eoo());
    actionElements = privs[0].Obj()["actions"].Array();
    ASSERT_EQUALS(2U, actionElements.size());
    ASSERT_EQUALS("find", actionElements[0].String());
    ASSERT_EQUALS("insert", actionElements[1].String());

    roles = roleDoc["roles"].Array();
    ASSERT_EQUALS(0U, roles.size());
}
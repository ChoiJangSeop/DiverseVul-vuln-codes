void addDbAdminAnyDbPrivileges(PrivilegeVector* privileges) {
    Privilege::addPrivilegeToPrivilegeVector(
        privileges, Privilege(ResourcePattern::forClusterResource(), ActionType::listDatabases));
    Privilege::addPrivilegeToPrivilegeVector(
        privileges, Privilege(ResourcePattern::forAnyNormalResource(), dbAdminRoleActions));
    Privilege::addPrivilegeToPrivilegeVector(
        privileges,
        Privilege(ResourcePattern::forCollectionName("system.indexes"), readRoleActions));
    Privilege::addPrivilegeToPrivilegeVector(
        privileges,
        Privilege(ResourcePattern::forCollectionName("system.namespaces"), readRoleActions));
    ActionSet profileActions = readRoleActions;
    profileActions.addAction(ActionType::convertToCapped);
    profileActions.addAction(ActionType::createCollection);
    profileActions.addAction(ActionType::dropCollection);
    Privilege::addPrivilegeToPrivilegeVector(
        privileges,
        Privilege(ResourcePattern::forCollectionName("system.profile"), profileActions));
}
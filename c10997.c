void addDbAdminAnyDbPrivileges(PrivilegeVector* privileges) {
    Privilege::addPrivilegeToPrivilegeVector(
        privileges, Privilege(ResourcePattern::forClusterResource(), ActionType::listDatabases));
    Privilege::addPrivilegeToPrivilegeVector(
        privileges, Privilege(ResourcePattern::forAnyNormalResource(), dbAdminRoleActions));
    ActionSet profileActions = readRoleActions;
    profileActions.addAction(ActionType::convertToCapped);
    profileActions.addAction(ActionType::createCollection);
    profileActions.addAction(ActionType::dropCollection);
    Privilege::addPrivilegeToPrivilegeVector(
        privileges,
        Privilege(ResourcePattern::forCollectionName("system.profile"), profileActions));
}
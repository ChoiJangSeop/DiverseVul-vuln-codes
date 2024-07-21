Status RoleGraph::getBSONForRole(RoleGraph* graph,
                                 const RoleName& roleName,
                                 mutablebson::Element result) try {
    if (!graph->roleExists(roleName)) {
        return Status(ErrorCodes::RoleNotFound,
                      str::stream() << roleName.getFullName() << "does not name an existing role");
    }
    std::string id = str::stream() << roleName.getDB() << "." << roleName.getRole();
    uassertStatusOK(result.appendString("_id", id));
    uassertStatusOK(
        result.appendString(AuthorizationManager::ROLE_NAME_FIELD_NAME, roleName.getRole()));
    uassertStatusOK(
        result.appendString(AuthorizationManager::ROLE_DB_FIELD_NAME, roleName.getDB()));

    // Build privileges array
    mutablebson::Element privilegesArrayElement =
        result.getDocument().makeElementArray("privileges");
    uassertStatusOK(result.pushBack(privilegesArrayElement));
    const PrivilegeVector& privileges = graph->getDirectPrivileges(roleName);
    uassertStatusOK(Privilege::getBSONForPrivileges(privileges, privilegesArrayElement));

    // Build roles array
    mutablebson::Element rolesArrayElement = result.getDocument().makeElementArray("roles");
    uassertStatusOK(result.pushBack(rolesArrayElement));
    for (RoleNameIterator roles = graph->getDirectSubordinates(roleName); roles.more();
         roles.next()) {
        const RoleName& subRole = roles.get();
        mutablebson::Element roleObj = result.getDocument().makeElementObject("");
        uassertStatusOK(
            roleObj.appendString(AuthorizationManager::ROLE_NAME_FIELD_NAME, subRole.getRole()));
        uassertStatusOK(
            roleObj.appendString(AuthorizationManager::ROLE_DB_FIELD_NAME, subRole.getDB()));
        uassertStatusOK(rolesArrayElement.pushBack(roleObj));
    }

    return Status::OK();
} catch (...) {
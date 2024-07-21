Status AuthorizationManagerImpl::_initializeUserFromPrivilegeDocument(User* user,
                                                                      const BSONObj& privDoc) {
    V2UserDocumentParser parser;
    std::string userName = parser.extractUserNameFromUserDocument(privDoc);
    if (userName != user->getName().getUser()) {
        return Status(ErrorCodes::BadValue,
                      mongoutils::str::stream() << "User name from privilege document \""
                                                << userName
                                                << "\" doesn't match name of provided User \""
                                                << user->getName().getUser()
                                                << "\"");
    }

    Status status = parser.initializeUserCredentialsFromUserDocument(user, privDoc);
    if (!status.isOK()) {
        return status;
    }
    status = parser.initializeUserRolesFromUserDocument(privDoc, user);
    if (!status.isOK()) {
        return status;
    }
    status = parser.initializeUserIndirectRolesFromUserDocument(privDoc, user);
    if (!status.isOK()) {
        return status;
    }
    status = parser.initializeUserPrivilegesFromUserDocument(privDoc, user);
    if (!status.isOK()) {
        return status;
    }
    status = parser.initializeAuthenticationRestrictionsFromUserDocument(privDoc, user);
    if (!status.isOK()) {
        return status;
    }

    return Status::OK();
}
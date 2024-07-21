Status V2UserDocumentParser::checkValidUserDocument(const BSONObj& doc) const {
    BSONElement userElement = doc[AuthorizationManager::USER_NAME_FIELD_NAME];
    BSONElement userDBElement = doc[AuthorizationManager::USER_DB_FIELD_NAME];
    BSONElement credentialsElement = doc[CREDENTIALS_FIELD_NAME];
    BSONElement rolesElement = doc[ROLES_FIELD_NAME];

    // Validate the "user" element.
    if (userElement.type() != String)
        return _badValue("User document needs 'user' field to be a string");
    if (userElement.valueStringData().empty())
        return _badValue("User document needs 'user' field to be non-empty");

    // Validate the "db" element
    if (userDBElement.type() != String || userDBElement.valueStringData().empty()) {
        return _badValue("User document needs 'db' field to be a non-empty string");
    }
    StringData userDBStr = userDBElement.valueStringData();
    if (!NamespaceString::validDBName(userDBStr, NamespaceString::DollarInDbNameBehavior::Allow) &&
        userDBStr != "$external") {
        return _badValue(mongoutils::str::stream() << "'" << userDBStr
                                                   << "' is not a valid value for the db field.");
    }

    // Validate the "credentials" element
    if (credentialsElement.eoo()) {
        return _badValue("User document needs 'credentials' object");
    }
    if (credentialsElement.type() != Object) {
        return _badValue("User document needs 'credentials' field to be an object");
    }

    BSONObj credentialsObj = credentialsElement.Obj();
    if (credentialsObj.isEmpty()) {
        return _badValue("User document needs 'credentials' field to be a non-empty object");
    }
    if (userDBStr == "$external") {
        BSONElement externalElement = credentialsObj[MONGODB_EXTERNAL_CREDENTIAL_FIELD_NAME];
        if (externalElement.eoo() || externalElement.type() != Bool || !externalElement.Bool()) {
            return _badValue(
                "User documents for users defined on '$external' must have "
                "'credentials' field set to {external: true}");
        }
    } else {
        const auto validateScram = [&credentialsObj](const auto& fieldName) {
            auto scramElement = credentialsObj[fieldName];

            if (scramElement.eoo()) {
                return Status(ErrorCodes::NoSuchKey,
                              str::stream() << fieldName << " does not exist");
            }
            if (scramElement.type() != Object) {
                return _badValue(str::stream() << fieldName
                                               << " credential must be an object, if present");
            }
            return Status::OK();
        };

        const auto sha1status = validateScram(SCRAMSHA1_CREDENTIAL_FIELD_NAME);
        if (!sha1status.isOK() && (sha1status.code() != ErrorCodes::NoSuchKey)) {
            return sha1status;
        }
        const auto sha256status = validateScram(SCRAMSHA256_CREDENTIAL_FIELD_NAME);
        if (!sha256status.isOK() && (sha256status.code() != ErrorCodes::NoSuchKey)) {
            return sha256status;
        }

        if (!sha1status.isOK() && !sha256status.isOK()) {
            return _badValue(
                "User document must provide credentials for all "
                "non-external users");
        }
    }

    // Validate the "roles" element.
    Status status = _checkV2RolesArray(rolesElement);
    if (!status.isOK())
        return status;

    // Validate the "authenticationRestrictions" element.
    status = initializeAuthenticationRestrictionsFromUserDocument(doc, nullptr);
    if (!status.isOK()) {
        return status;
    }

    return Status::OK();
}
Status OplogApplicationChecks::checkAuthForCommand(OperationContext* opCtx,
                                                   const std::string& dbname,
                                                   const BSONObj& cmdObj,
                                                   OplogApplicationValidity validity) {
    AuthorizationSession* authSession = AuthorizationSession::get(opCtx->getClient());
    if (validity == OplogApplicationValidity::kNeedsSuperuser) {
        std::vector<Privilege> universalPrivileges;
        RoleGraph::generateUniversalPrivileges(&universalPrivileges);
        if (!authSession->isAuthorizedForPrivileges(universalPrivileges)) {
            return Status(ErrorCodes::Unauthorized, "Unauthorized");
        }
        return Status::OK();
    }
    if (validity == OplogApplicationValidity::kNeedsForceAndUseUUID) {
        if (!authSession->isAuthorizedForActionsOnResource(
                ResourcePattern::forClusterResource(),
                {ActionType::forceUUID, ActionType::useUUID})) {
            return Status(ErrorCodes::Unauthorized, "Unauthorized");
        }
        validity = OplogApplicationValidity::kOk;
    }
    if (validity == OplogApplicationValidity::kNeedsUseUUID) {
        if (!authSession->isAuthorizedForActionsOnResource(ResourcePattern::forClusterResource(),
                                                           ActionType::useUUID)) {
            return Status(ErrorCodes::Unauthorized, "Unauthorized");
        }
        validity = OplogApplicationValidity::kOk;
    }
    fassert(40314, validity == OplogApplicationValidity::kOk);

    boost::optional<DisableDocumentValidation> maybeDisableValidation;
    if (shouldBypassDocumentValidationForCommand(cmdObj))
        maybeDisableValidation.emplace(opCtx);

    const bool alwaysUpsert =
        cmdObj.hasField("alwaysUpsert") ? cmdObj["alwaysUpsert"].trueValue() : true;

    checkBSONType(BSONType::Array, cmdObj.firstElement());
    for (const BSONElement& e : cmdObj.firstElement().Array()) {
        checkBSONType(BSONType::Object, e);
        Status status = OplogApplicationChecks::checkOperationAuthorization(
            opCtx, dbname, e.Obj(), authSession, alwaysUpsert);
        if (!status.isOK()) {
            return status;
        }
    }

    BSONElement preconditions = cmdObj["preCondition"];
    if (!preconditions.eoo()) {
        for (const BSONElement& precondition : preconditions.Array()) {
            checkBSONType(BSONType::Object, precondition);
            BSONElement nsElem = precondition.Obj()["ns"];
            checkBSONType(BSONType::String, nsElem);
            NamespaceString nss(nsElem.checkAndGetStringData());

            if (!authSession->isAuthorizedForActionsOnResource(
                    ResourcePattern::forExactNamespace(nss), ActionType::find)) {
                return Status(ErrorCodes::Unauthorized, "Unauthorized to check precondition");
            }
        }
    }

    return Status::OK();
}
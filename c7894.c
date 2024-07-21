void AuthorizationSessionImpl::_refreshUserInfoAsNeeded(OperationContext* opCtx) {
    AuthorizationManager& authMan = getAuthorizationManager();
    UserSet::iterator it = _authenticatedUsers.begin();

    while (it != _authenticatedUsers.end()) {
        auto& user = *it;
        if (!user->isValid()) {
            // The user is invalid, so make sure that we erase it from _authenticateUsers at the
            // end of this block.
            auto removeGuard = makeGuard([&] { _authenticatedUsers.removeAt(it++); });

            // Make a good faith effort to acquire an up-to-date user object, since the one
            // we've cached is marked "out-of-date."
            UserName name = user->getName();
            UserHandle updatedUser;

            auto swUser = authMan.acquireUser(opCtx, name);
            auto& status = swUser.getStatus();
            switch (status.code()) {
                case ErrorCodes::OK: {
                    updatedUser = std::move(swUser.getValue());
                    try {
                        const auto& restrictionSet =
                            updatedUser->getRestrictions();  // Owned by updatedUser
                        invariant(opCtx->getClient());
                        Status restrictionStatus = restrictionSet.validate(
                            RestrictionEnvironment::get(*opCtx->getClient()));
                        if (!restrictionStatus.isOK()) {
                            log() << "Removed user " << name
                                  << " with unmet authentication restrictions from session cache of"
                                  << " user information. Restriction failed because: "
                                  << restrictionStatus.reason();
                            // If we remove from the UserSet, we cannot increment the iterator.
                            continue;
                        }
                    } catch (...) {
                        log() << "Evaluating authentication restrictions for " << name
                              << " resulted in an unknown exception. Removing user from the"
                              << " session cache.";
                        continue;
                    }

                    // Success! Replace the old User object with the updated one.
                    removeGuard.dismiss();
                    _authenticatedUsers.replaceAt(it, std::move(updatedUser));
                    LOG(1) << "Updated session cache of user information for " << name;
                    break;
                }
                case ErrorCodes::UserNotFound: {
                    // User does not exist anymore; remove it from _authenticatedUsers.
                    log() << "Removed deleted user " << name
                          << " from session cache of user information.";
                    continue;  // No need to advance "it" in this case.
                }
                case ErrorCodes::UnsupportedFormat: {
                    // An auth subsystem has explicitly indicated a failure.
                    log() << "Removed user " << name
                          << " from session cache of user information because of refresh failure:"
                          << " '" << status << "'.";
                    continue;  // No need to advance "it" in this case.
                }
                default:
                    // Unrecognized error; assume that it's transient, and continue working with the
                    // out-of-date privilege data.
                    warning() << "Could not fetch updated user privilege information for " << name
                              << "; continuing to use old information.  Reason is "
                              << redact(status);
                    removeGuard.dismiss();
                    break;
            }
        }
        ++it;
    }
    _buildAuthenticatedRolesVector();
}
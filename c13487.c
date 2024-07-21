otError Commissioner::AddJoiner(const Mac::ExtAddress *aEui64, const char *aPskd, uint32_t aTimeout)
{
    otError error = OT_ERROR_NO_BUFS;

    VerifyOrExit(mState == OT_COMMISSIONER_STATE_ACTIVE, error = OT_ERROR_INVALID_STATE);

    VerifyOrExit(strlen(aPskd) <= Dtls::kPskMaxLength, error = OT_ERROR_INVALID_ARGS);
    RemoveJoiner(aEui64, 0); // remove immediately

    for (Joiner *joiner = &mJoiners[0]; joiner < OT_ARRAY_END(mJoiners); joiner++)
    {
        if (joiner->mValid)
        {
            continue;
        }

        if (aEui64 != NULL)
        {
            joiner->mEui64 = *aEui64;
            joiner->mAny   = false;
        }
        else
        {
            joiner->mAny = true;
        }

        (void)strlcpy(joiner->mPsk, aPskd, sizeof(joiner->mPsk));
        joiner->mValid          = true;
        joiner->mExpirationTime = TimerMilli::GetNow() + Time::SecToMsec(aTimeout);

        UpdateJoinerExpirationTimer();

        SendCommissionerSet();

        otLogInfoMeshCoP("Added Joiner (%s, %s)", (aEui64 != NULL) ? aEui64->ToString().AsCString() : "*", aPskd);

        ExitNow(error = OT_ERROR_NONE);
    }

exit:
    return error;
}
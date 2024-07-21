void recovery_cipher_finalize(void)
{
    static char CONFIDENTIAL new_mnemonic[MNEMONIC_BUF] = "";
    static char CONFIDENTIAL temp_word[CURRENT_WORD_BUF];
    volatile bool auto_completed = true;

    /* Attempt to autocomplete each word */
    char *tok = strtok(mnemonic, " ");

    while(tok) {
        strlcpy(temp_word, tok, CURRENT_WORD_BUF);

        auto_completed &= attempt_auto_complete(temp_word);

        strlcat(new_mnemonic, temp_word, MNEMONIC_BUF);
        strlcat(new_mnemonic, " ", MNEMONIC_BUF);

        tok = strtok(NULL, " ");
    }
    memzero(temp_word, sizeof(temp_word));

    if (!auto_completed && !enforce_wordlist) {
        if (!dry_run) {
            storage_reset();
        }
        fsm_sendFailure(FailureType_Failure_SyntaxError,
                        "Words were not entered correctly. Make sure you are using the substition cipher.");
        awaiting_character = false;
        layoutHome();
        return;
    }

    /* Truncate additional space at the end */
    new_mnemonic[strlen(new_mnemonic) - 1] = '\0';

    if (!dry_run && (!enforce_wordlist || mnemonic_check(new_mnemonic))) {
        storage_setMnemonic(new_mnemonic);
        memzero(new_mnemonic, sizeof(new_mnemonic));
        if (!enforce_wordlist) {
            // not enforcing => mark storage as imported
            storage_setImported(true);
        }
        storage_commit();
        fsm_sendSuccess("Device recovered");
    } else if (dry_run) {
        bool match = storage_isInitialized() && storage_containsMnemonic(new_mnemonic);
        if (match) {
            review(ButtonRequestType_ButtonRequest_Other, "Recovery Dry Run",
                   "The seed is valid and MATCHES the one in the device.");
            fsm_sendSuccess("The seed is valid and matches the one in the device.");
        } else if (mnemonic_check(new_mnemonic)) {
            review(ButtonRequestType_ButtonRequest_Other, "Recovery Dry Run",
                   "The seed is valid, but DOES NOT MATCH the one in the device.");
            fsm_sendFailure(FailureType_Failure_Other,
                            "The seed is valid, but does not match the one in the device.");
        } else {
            review(ButtonRequestType_ButtonRequest_Other, "Recovery Dry Run",
                   "The seed is INVALID, and DOES NOT MATCH the one in the device.");
            fsm_sendFailure(FailureType_Failure_Other,
                            "The seed is invalid, and does not match the one in the device.");
        }
        memzero(new_mnemonic, sizeof(new_mnemonic));
    } else {
        session_clear(true);
        fsm_sendFailure(FailureType_Failure_SyntaxError,
                        "Invalid mnemonic, are words in correct order?");
        recovery_abort();
    }

    memzero(new_mnemonic, sizeof(new_mnemonic));
    awaiting_character = false;
    memzero(mnemonic, sizeof(mnemonic));
    memzero(cipher, sizeof(cipher));
    layoutHome();
}
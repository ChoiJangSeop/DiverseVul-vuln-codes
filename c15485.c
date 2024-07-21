void recovery_cipher_init(bool passphrase_protection, bool pin_protection,
                          const char *language, const char *label, bool _enforce_wordlist,
                          uint32_t _auto_lock_delay_ms, uint32_t _u2f_counter, bool _dry_run)
{
    enforce_wordlist = _enforce_wordlist;
    dry_run = _dry_run;

    if (!dry_run) {
        if (pin_protection) {
            if (!change_pin()) {
                recovery_abort();
                fsm_sendFailure(FailureType_Failure_ActionCancelled, "PINs do not match");
                layoutHome();
                return;
            }
        } else {
            storage_setPin("");
        }

        storage_setPassphraseProtected(passphrase_protection);
        storage_setLanguage(language);
        storage_setLabel(label);
        storage_setAutoLockDelayMs(_auto_lock_delay_ms);
        storage_setU2FCounter(_u2f_counter);
    } else if (!pin_protect("Enter Your PIN")) {
        layoutHome();
        return;
    }

    if (!confirm(ButtonRequestType_ButtonRequest_Other,
                 dry_run ? "Recovery Dry Run" : "Recovery",
                 "When entering your recovery seed, use the substitution cipher "
                 "and check that each word shows up correctly on the screen.")) {
        fsm_sendFailure(FailureType_Failure_ActionCancelled, "Recovery cancelled");
        if (!dry_run)
            storage_reset();
        layoutHome();
        return;
    }

    /* Clear mnemonic */
    memset(mnemonic, 0, sizeof(mnemonic) / sizeof(char));

    /* Set to recovery cipher mode and generate and show next cipher */
    awaiting_character = true;
    next_character();
}
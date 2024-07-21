static void recovery_abort(void) {
    if (!dry_run) {
        storage_reset();
    }

    awaiting_character = false;
    memzero(mnemonic, sizeof(mnemonic));
    memzero(cipher, sizeof(cipher));
}
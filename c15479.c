void next_character(void)
{
    /* Scramble cipher */
    strlcpy(cipher, english_alphabet, ENGLISH_ALPHABET_BUF);
    random_permute_char(cipher, strlen(cipher));

    static char CONFIDENTIAL current_word[CURRENT_WORD_BUF];
    get_current_word(current_word);

    /* Words should never be longer than 4 characters */
    if (strlen(current_word) > 4) {
        memzero(current_word, sizeof(current_word));

        recovery_abort();
        fsm_sendFailure(FailureType_Failure_SyntaxError,
                        "Words were not entered correctly. Make sure you are using the substition cipher.");
        layoutHome();
        return;
    }

    CharacterRequest resp;
    memset(&resp, 0, sizeof(CharacterRequest));

    resp.word_pos = get_current_word_pos();
    resp.character_pos = strlen(current_word);

    msg_write(MessageType_MessageType_CharacterRequest, &resp);

    /* Attempt to auto complete if we have at least 3 characters */
    bool auto_completed = false;
    if (strlen(current_word) >= 3) {
        auto_completed = attempt_auto_complete(current_word);
    }

#if DEBUG_LINK
    if (auto_completed) {
        strlcpy(auto_completed_word, current_word, CURRENT_WORD_BUF);
    } else {
        auto_completed_word[0] = '\0';
    }
#endif

    /* Format current word and display it along with cipher */
    format_current_word(current_word, auto_completed);

    /* Show cipher and partial word */
    layout_cipher(current_word, cipher);
    memzero(current_word, sizeof(current_word));
}
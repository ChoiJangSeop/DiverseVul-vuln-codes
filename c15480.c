bool recovery_cipher_abort(void)
{
    if (awaiting_character) {
        awaiting_character = false;
        return true;
    }
    return false;
}
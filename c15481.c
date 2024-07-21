void recovery_delete_character(void)
{
    if(strlen(mnemonic) > 0)
    {
        mnemonic[strlen(mnemonic) - 1] = '\0';
    }

    next_character();
}
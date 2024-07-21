vte_sequence_handler_multiple(VteTerminal *terminal,
                              GValueArray *params,
                              VteTerminalSequenceHandler handler)
{
        vte_sequence_handler_multiple_limited(terminal, params, handler, G_MAXLONG);
}
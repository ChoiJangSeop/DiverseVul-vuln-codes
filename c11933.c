log_tx_piggyback (const char *q, const char qtype[2], const char *control)
{
    string("txpb ");
    logtype (qtype);
    space ();
    name (q);
    space ();
    name (control);
    line ();
}
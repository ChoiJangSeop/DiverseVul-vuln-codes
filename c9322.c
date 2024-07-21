shutdown_mib(void)
{
    unload_all_mibs();
    if (tree_top) {
        if (tree_top->label)
            SNMP_FREE(tree_top->label);
        SNMP_FREE(tree_top);
    }
    tree_head = NULL;
    Mib = NULL;
    if (_mibindexes) {
        int i;
        for (i = 0; i < _mibindex; ++i)
            SNMP_FREE(_mibindexes[i]);
        free(_mibindexes);
        _mibindex = 0;
        _mibindex_max = 0;
        _mibindexes = NULL;
    }
    if (Prefix != NULL && Prefix != &Standard_Prefix[0])
        SNMP_FREE(Prefix);
    if (Prefix)
        Prefix = NULL;
    SNMP_FREE(confmibs);
    SNMP_FREE(confmibdir);
}
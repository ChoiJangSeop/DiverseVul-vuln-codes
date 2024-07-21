int xml_init(modsec_rec *msr, char **error_msg) {
    if (error_msg == NULL) return -1;
    *error_msg = NULL;

    msr->xml = apr_pcalloc(msr->mp, sizeof(xml_data));
    if (msr->xml == NULL) return -1;

    return 1;
}
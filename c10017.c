void options_apply() { /* apply default/validated configuration */
    unsigned num=0;
    SERVICE_OPTIONS *section;

    CRYPTO_THREAD_write_lock(stunnel_locks[LOCK_SECTIONS]);

    memcpy(&global_options, &new_global_options, sizeof(GLOBAL_OPTIONS));

    /* service_options are used for inetd mode and to enumerate services */
    for(section=new_service_options.next; section; section=section->next)
        section->section_number=num++;
    memcpy(&service_options, &new_service_options, sizeof(SERVICE_OPTIONS));
    number_of_sections=num;

    CRYPTO_THREAD_unlock(stunnel_locks[LOCK_SECTIONS]);
}
static int xmlInitializeDict(void) {
    if (xmlDictInitialized)
        return(1);

    if ((xmlDictMutex = xmlNewRMutex()) == NULL)
        return(0);

    xmlDictInitialized = 1;
    return(1);
}
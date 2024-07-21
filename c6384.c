Avahi * Avahi::self()
{
    #ifdef ENABLE_KDE_SUPPORT
    return instance;
    #else
    static Avahi *instance=0;
    if(!instance) {
        instance=new Avahi;
    }
    return instance;
    #endif
}
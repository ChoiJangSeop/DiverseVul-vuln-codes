void ahci_uninit(AHCIState *s)
{
    g_free(s->dev);
}
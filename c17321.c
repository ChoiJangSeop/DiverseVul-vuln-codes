void OPENSSL_LH_flush(OPENSSL_LHASH *lh)
{
    unsigned int i;
    OPENSSL_LH_NODE *n, *nn;

    if (lh == NULL)
        return;

    for (i = 0; i < lh->num_nodes; i++) {
        n = lh->b[i];
        while (n != NULL) {
            nn = n->next;
            OPENSSL_free(n);
            n = nn;
        }
        lh->b[i] = NULL;
    }
}
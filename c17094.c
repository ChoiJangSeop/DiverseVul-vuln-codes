void *GENERAL_NAME_get0_value(const GENERAL_NAME *a, int *ptype)
{
    if (ptype)
        *ptype = a->type;
    switch (a->type) {
    case GEN_X400:
    case GEN_EDIPARTY:
        return a->d.other;

    case GEN_OTHERNAME:
        return a->d.otherName;

    case GEN_EMAIL:
    case GEN_DNS:
    case GEN_URI:
        return a->d.ia5;

    case GEN_DIRNAME:
        return a->d.dirn;

    case GEN_IPADD:
        return a->d.ip;

    case GEN_RID:
        return a->d.rid;

    default:
        return NULL;
    }
}
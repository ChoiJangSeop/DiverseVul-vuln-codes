int asn1_ex_i2c(ASN1_VALUE **pval, unsigned char *cout, int *putype,
                const ASN1_ITEM *it)
{
    ASN1_BOOLEAN *tbool = NULL;
    ASN1_STRING *strtmp;
    ASN1_OBJECT *otmp;
    int utype;
    const unsigned char *cont;
    unsigned char c;
    int len;
    const ASN1_PRIMITIVE_FUNCS *pf;
    pf = it->funcs;
    if (pf && pf->prim_i2c)
        return pf->prim_i2c(pval, cout, putype, it);

    /* Should type be omitted? */
    if ((it->itype != ASN1_ITYPE_PRIMITIVE)
        || (it->utype != V_ASN1_BOOLEAN)) {
        if (!*pval)
            return -1;
    }

    if (it->itype == ASN1_ITYPE_MSTRING) {
        /* If MSTRING type set the underlying type */
        strtmp = (ASN1_STRING *)*pval;
        utype = strtmp->type;
        *putype = utype;
    } else if (it->utype == V_ASN1_ANY) {
        /* If ANY set type and pointer to value */
        ASN1_TYPE *typ;
        typ = (ASN1_TYPE *)*pval;
        utype = typ->type;
        *putype = utype;
        pval = &typ->value.asn1_value;
    } else
        utype = *putype;

    switch (utype) {
    case V_ASN1_OBJECT:
        otmp = (ASN1_OBJECT *)*pval;
        cont = otmp->data;
        len = otmp->length;
        break;

    case V_ASN1_NULL:
        cont = NULL;
        len = 0;
        break;

    case V_ASN1_BOOLEAN:
        tbool = (ASN1_BOOLEAN *)pval;
        if (*tbool == -1)
            return -1;
        if (it->utype != V_ASN1_ANY) {
            /*
             * Default handling if value == size field then omit
             */
            if (*tbool && (it->size > 0))
                return -1;
            if (!*tbool && !it->size)
                return -1;
        }
        c = (unsigned char)*tbool;
        cont = &c;
        len = 1;
        break;

    case V_ASN1_BIT_STRING:
        return i2c_ASN1_BIT_STRING((ASN1_BIT_STRING *)*pval,
                                   cout ? &cout : NULL);
        break;

    case V_ASN1_INTEGER:
    case V_ASN1_NEG_INTEGER:
    case V_ASN1_ENUMERATED:
    case V_ASN1_NEG_ENUMERATED:
        /*
         * These are all have the same content format as ASN1_INTEGER
         */
        return i2c_ASN1_INTEGER((ASN1_INTEGER *)*pval, cout ? &cout : NULL);
        break;

    case V_ASN1_OCTET_STRING:
    case V_ASN1_NUMERICSTRING:
    case V_ASN1_PRINTABLESTRING:
    case V_ASN1_T61STRING:
    case V_ASN1_VIDEOTEXSTRING:
    case V_ASN1_IA5STRING:
    case V_ASN1_UTCTIME:
    case V_ASN1_GENERALIZEDTIME:
    case V_ASN1_GRAPHICSTRING:
    case V_ASN1_VISIBLESTRING:
    case V_ASN1_GENERALSTRING:
    case V_ASN1_UNIVERSALSTRING:
    case V_ASN1_BMPSTRING:
    case V_ASN1_UTF8STRING:
    case V_ASN1_SEQUENCE:
    case V_ASN1_SET:
    default:
        /* All based on ASN1_STRING and handled the same */
        strtmp = (ASN1_STRING *)*pval;
        /* Special handling for NDEF */
        if ((it->size == ASN1_TFLG_NDEF)
            && (strtmp->flags & ASN1_STRING_FLAG_NDEF)) {
            if (cout) {
                strtmp->data = cout;
                strtmp->length = 0;
            }
            /* Special return code */
            return -2;
        }
        cont = strtmp->data;
        len = strtmp->length;

        break;

    }
    if (cout && len)
        memcpy(cout, cont, len);
    return len;
}
int ASN1_TYPE_cmp(const ASN1_TYPE *a, const ASN1_TYPE *b)
{
    int result = -1;

    if (!a || !b || a->type != b->type)
        return -1;

    switch (a->type) {
    case V_ASN1_OBJECT:
        result = OBJ_cmp(a->value.object, b->value.object);
        break;
    case V_ASN1_NULL:
        result = 0;             /* They do not have content. */
        break;
    case V_ASN1_INTEGER:
    case V_ASN1_NEG_INTEGER:
    case V_ASN1_ENUMERATED:
    case V_ASN1_NEG_ENUMERATED:
    case V_ASN1_BIT_STRING:
    case V_ASN1_OCTET_STRING:
    case V_ASN1_SEQUENCE:
    case V_ASN1_SET:
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
    case V_ASN1_OTHER:
    default:
        result = ASN1_STRING_cmp((ASN1_STRING *)a->value.ptr,
                                 (ASN1_STRING *)b->value.ptr);
        break;
    }

    return result;
}
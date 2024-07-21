decode_sequence(const uint8_t *asn1, size_t len, const struct seq_info *seq,
                void *val)
{
    krb5_error_code ret;
    const uint8_t *contents;
    size_t i, j, clen;
    taginfo t;

    assert(seq->n_fields > 0);
    for (i = 0; i < seq->n_fields; i++) {
        if (len == 0)
            break;
        ret = get_tag(asn1, len, &t, &contents, &clen, &asn1, &len);
        if (ret)
            goto error;
        /*
         * Find the applicable sequence field.  This logic is a little
         * oversimplified; we could match an element to an optional extensible
         * choice or optional stored-DER type when we ought to match a
         * subsequent non-optional field.  But it's unwise and (hopefully) very
         * rare for ASN.1 modules to require such precision.
         */
        for (; i < seq->n_fields; i++) {
            if (check_atype_tag(seq->fields[i], &t))
                break;
            ret = omit_atype(seq->fields[i], val);
            if (ret)
                goto error;
        }
        /* We currently model all sequences as extensible.  We should consider
         * changing this before making the encoder visible to plugins. */
        if (i == seq->n_fields)
            break;
        ret = decode_atype(&t, contents, clen, seq->fields[i], val);
        if (ret)
            goto error;
    }
    /* Initialize any fields in the C object which were not accounted for in
     * the sequence.  Error out if any of them aren't optional. */
    for (; i < seq->n_fields; i++) {
        ret = omit_atype(seq->fields[i], val);
        if (ret)
            goto error;
    }
    return 0;

error:
    /* Free what we've decoded so far.  Free pointers in a second pass in
     * case multiple fields refer to the same pointer. */
    for (j = 0; j < i; j++)
        free_atype(seq->fields[j], val);
    for (j = 0; j < i; j++)
        free_atype_ptr(seq->fields[j], val);
    return ret;
}
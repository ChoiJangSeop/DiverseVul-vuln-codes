decode_atype(const taginfo *t, const uint8_t *asn1, size_t len,
             const struct atype_info *a, void *val)
{
    krb5_error_code ret;

    switch (a->type) {
    case atype_fn: {
        const struct fn_info *fn = a->tinfo;
        assert(fn->dec != NULL);
        return fn->dec(t, asn1, len, val);
    }
    case atype_sequence:
        return decode_sequence(asn1, len, a->tinfo, val);
    case atype_ptr: {
        const struct ptr_info *ptrinfo = a->tinfo;
        void *ptr = LOADPTR(val, ptrinfo);
        assert(ptrinfo->basetype != NULL);
        if (ptr != NULL) {
            /* Container was already allocated by a previous sequence field. */
            return decode_atype(t, asn1, len, ptrinfo->basetype, ptr);
        } else {
            ret = decode_atype_to_ptr(t, asn1, len, ptrinfo->basetype, &ptr);
            if (ret)
                return ret;
            STOREPTR(ptr, ptrinfo, val);
            break;
        }
    }
    case atype_offset: {
        const struct offset_info *off = a->tinfo;
        assert(off->basetype != NULL);
        return decode_atype(t, asn1, len, off->basetype,
                            (char *)val + off->dataoff);
    }
    case atype_optional: {
        const struct optional_info *opt = a->tinfo;
        return decode_atype(t, asn1, len, opt->basetype, val);
    }
    case atype_counted: {
        const struct counted_info *counted = a->tinfo;
        void *dataptr = (char *)val + counted->dataoff;
        size_t count;
        assert(counted->basetype != NULL);
        ret = decode_cntype(t, asn1, len, counted->basetype, dataptr, &count);
        if (ret)
            return ret;
        return store_count(count, counted, val);
    }
    case atype_tagged_thing: {
        const struct tagged_info *tag = a->tinfo;
        taginfo inner_tag;
        const taginfo *tp = t;
        const uint8_t *rem;
        size_t rlen;
        if (!tag->implicit) {
            ret = get_tag(asn1, len, &inner_tag, &asn1, &len, &rem, &rlen);
            if (ret)
                return ret;
            /* Note: we don't check rlen (it should be 0). */
            tp = &inner_tag;
            if (!check_atype_tag(tag->basetype, tp))
                return ASN1_BAD_ID;
        }
        return decode_atype(tp, asn1, len, tag->basetype, val);
    }
    case atype_bool: {
        intmax_t intval;
        ret = k5_asn1_decode_bool(asn1, len, &intval);
        if (ret)
            return ret;
        return store_int(intval, a->size, val);
    }
    case atype_int: {
        intmax_t intval;
        ret = k5_asn1_decode_int(asn1, len, &intval);
        if (ret)
            return ret;
        return store_int(intval, a->size, val);
    }
    case atype_uint: {
        uintmax_t intval;
        ret = k5_asn1_decode_uint(asn1, len, &intval);
        if (ret)
            return ret;
        return store_uint(intval, a->size, val);
    }
    case atype_int_immediate: {
        const struct immediate_info *imm = a->tinfo;
        intmax_t intval;
        ret = k5_asn1_decode_int(asn1, len, &intval);
        if (ret)
            return ret;
        if (intval != imm->val && imm->err != 0)
            return imm->err;
        break;
    }
    default:
        /* Null-terminated sequence types are handled in decode_atype_to_ptr,
         * since they create variable-sized objects. */
        assert(a->type != atype_nullterm_sequence_of);
        assert(a->type != atype_nonempty_nullterm_sequence_of);
        assert(a->type > atype_min);
        assert(a->type < atype_max);
        abort();
    }
    return 0;
}
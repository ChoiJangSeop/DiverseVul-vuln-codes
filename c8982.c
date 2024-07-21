static void nodeConstruct(struct SaveNode* node, tr_variant const* v, bool sort_dicts)
{
    node->isVisited = false;
    node->childIndex = 0;

    if (sort_dicts && tr_variantIsDict(v))
    {
        /* make node->sorted a sorted version of this dictionary */

        size_t const n = v->val.l.count;
        struct KeyIndex* tmp = tr_new(struct KeyIndex, n);

        for (size_t i = 0; i < n; i++)
        {
            tmp[i].val = v->val.l.vals + i;
            tmp[i].keystr = tr_quark_get_string(tmp[i].val->key, NULL);
        }

        qsort(tmp, n, sizeof(struct KeyIndex), compareKeyIndex);

        tr_variantInitDict(&node->sorted, n);

        for (size_t i = 0; i < n; ++i)
        {
            node->sorted.val.l.vals[i] = *tmp[i].val;
        }

        node->sorted.val.l.count = n;

        tr_free(tmp);

        node->v = &node->sorted;
    }
    else
    {
        node->v = v;
    }
}
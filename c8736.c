ast_for_suite(struct compiling *c, const node *n)
{
    /* suite: simple_stmt | NEWLINE INDENT stmt+ DEDENT */
    asdl_seq *seq;
    stmt_ty s;
    int i, total, num, end, pos = 0;
    node *ch;

    REQ(n, suite);

    total = num_stmts(n);
    seq = _Py_asdl_seq_new(total, c->c_arena);
    if (!seq)
        return NULL;
    if (TYPE(CHILD(n, 0)) == simple_stmt) {
        n = CHILD(n, 0);
        /* simple_stmt always ends with a NEWLINE,
           and may have a trailing SEMI
        */
        end = NCH(n) - 1;
        if (TYPE(CHILD(n, end - 1)) == SEMI)
            end--;
        /* loop by 2 to skip semi-colons */
        for (i = 0; i < end; i += 2) {
            ch = CHILD(n, i);
            s = ast_for_stmt(c, ch);
            if (!s)
                return NULL;
            asdl_seq_SET(seq, pos++, s);
        }
    }
    else {
        for (i = 2; i < (NCH(n) - 1); i++) {
            ch = CHILD(n, i);
            REQ(ch, stmt);
            num = num_stmts(ch);
            if (num == 1) {
                /* small_stmt or compound_stmt with only one child */
                s = ast_for_stmt(c, ch);
                if (!s)
                    return NULL;
                asdl_seq_SET(seq, pos++, s);
            }
            else {
                int j;
                ch = CHILD(ch, 0);
                REQ(ch, simple_stmt);
                for (j = 0; j < NCH(ch); j += 2) {
                    /* statement terminates with a semi-colon ';' */
                    if (NCH(CHILD(ch, j)) == 0) {
                        assert((j + 1) == NCH(ch));
                        break;
                    }
                    s = ast_for_stmt(c, CHILD(ch, j));
                    if (!s)
                        return NULL;
                    asdl_seq_SET(seq, pos++, s);
                }
            }
        }
    }
    assert(pos == seq->size);
    return seq;
}
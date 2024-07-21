ast_for_expr_stmt(struct compiling *c, const node *n)
{
    REQ(n, expr_stmt);
    /* expr_stmt: testlist_star_expr (annassign | augassign (yield_expr|testlist) |
                            ('=' (yield_expr|testlist_star_expr))*)
       annassign: ':' test ['=' test]
       testlist_star_expr: (test|star_expr) (',' test|star_expr)* [',']
       augassign: '+=' | '-=' | '*=' | '@=' | '/=' | '%=' | '&=' | '|=' | '^='
                | '<<=' | '>>=' | '**=' | '//='
       test: ... here starts the operator precedence dance
     */

    if (NCH(n) == 1) {
        expr_ty e = ast_for_testlist(c, CHILD(n, 0));
        if (!e)
            return NULL;

        return Expr(e, LINENO(n), n->n_col_offset,
                    n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else if (TYPE(CHILD(n, 1)) == augassign) {
        expr_ty expr1, expr2;
        operator_ty newoperator;
        node *ch = CHILD(n, 0);

        expr1 = ast_for_testlist(c, ch);
        if (!expr1)
            return NULL;
        if(!set_context(c, expr1, Store, ch))
            return NULL;
        /* set_context checks that most expressions are not the left side.
          Augmented assignments can only have a name, a subscript, or an
          attribute on the left, though, so we have to explicitly check for
          those. */
        switch (expr1->kind) {
            case Name_kind:
            case Attribute_kind:
            case Subscript_kind:
                break;
            default:
                ast_error(c, ch, "illegal expression for augmented assignment");
                return NULL;
        }

        ch = CHILD(n, 2);
        if (TYPE(ch) == testlist)
            expr2 = ast_for_testlist(c, ch);
        else
            expr2 = ast_for_expr(c, ch);
        if (!expr2)
            return NULL;

        newoperator = ast_for_augassign(c, CHILD(n, 1));
        if (!newoperator)
            return NULL;

        return AugAssign(expr1, newoperator, expr2, LINENO(n), n->n_col_offset,
                         n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
    else if (TYPE(CHILD(n, 1)) == annassign) {
        expr_ty expr1, expr2, expr3;
        node *ch = CHILD(n, 0);
        node *deep, *ann = CHILD(n, 1);
        int simple = 1;

        /* we keep track of parens to qualify (x) as expression not name */
        deep = ch;
        while (NCH(deep) == 1) {
            deep = CHILD(deep, 0);
        }
        if (NCH(deep) > 0 && TYPE(CHILD(deep, 0)) == LPAR) {
            simple = 0;
        }
        expr1 = ast_for_testlist(c, ch);
        if (!expr1) {
            return NULL;
        }
        switch (expr1->kind) {
            case Name_kind:
                if (forbidden_name(c, expr1->v.Name.id, n, 0)) {
                    return NULL;
                }
                expr1->v.Name.ctx = Store;
                break;
            case Attribute_kind:
                if (forbidden_name(c, expr1->v.Attribute.attr, n, 1)) {
                    return NULL;
                }
                expr1->v.Attribute.ctx = Store;
                break;
            case Subscript_kind:
                expr1->v.Subscript.ctx = Store;
                break;
            case List_kind:
                ast_error(c, ch,
                          "only single target (not list) can be annotated");
                return NULL;
            case Tuple_kind:
                ast_error(c, ch,
                          "only single target (not tuple) can be annotated");
                return NULL;
            default:
                ast_error(c, ch,
                          "illegal target for annotation");
                return NULL;
        }

        if (expr1->kind != Name_kind) {
            simple = 0;
        }
        ch = CHILD(ann, 1);
        expr2 = ast_for_expr(c, ch);
        if (!expr2) {
            return NULL;
        }
        if (NCH(ann) == 2) {
            return AnnAssign(expr1, expr2, NULL, simple,
                             LINENO(n), n->n_col_offset,
                             n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
        else {
            ch = CHILD(ann, 3);
            if (TYPE(ch) == testlist) {
                expr3 = ast_for_testlist(c, ch);
            }
            else {
                expr3 = ast_for_expr(c, ch);
            }
            if (!expr3) {
                return NULL;
            }
            return AnnAssign(expr1, expr2, expr3, simple,
                             LINENO(n), n->n_col_offset,
                             n->n_end_lineno, n->n_end_col_offset, c->c_arena);
        }
    }
    else {
        int i;
        asdl_seq *targets;
        node *value;
        expr_ty expression;

        /* a normal assignment */
        REQ(CHILD(n, 1), EQUAL);
        targets = _Py_asdl_seq_new(NCH(n) / 2, c->c_arena);
        if (!targets)
            return NULL;
        for (i = 0; i < NCH(n) - 2; i += 2) {
            expr_ty e;
            node *ch = CHILD(n, i);
            if (TYPE(ch) == yield_expr) {
                ast_error(c, ch, "assignment to yield expression not possible");
                return NULL;
            }
            e = ast_for_testlist(c, ch);
            if (!e)
              return NULL;

            /* set context to assign */
            if (!set_context(c, e, Store, CHILD(n, i)))
              return NULL;

            asdl_seq_SET(targets, i / 2, e);
        }
        value = CHILD(n, NCH(n) - 1);
        if (TYPE(value) == testlist_star_expr)
            expression = ast_for_testlist(c, value);
        else
            expression = ast_for_expr(c, value);
        if (!expression)
            return NULL;
        return Assign(targets, expression, LINENO(n), n->n_col_offset,
                      n->n_end_lineno, n->n_end_col_offset, c->c_arena);
    }
}
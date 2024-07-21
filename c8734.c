fstring_find_expr(const char **str, const char *end, int raw, int recurse_lvl,
                  expr_ty *expression, struct compiling *c, const node *n)
{
    /* Return -1 on error, else 0. */

    const char *expr_start;
    const char *expr_end;
    expr_ty simple_expression;
    expr_ty format_spec = NULL; /* Optional format specifier. */
    int conversion = -1; /* The conversion char. -1 if not specified. */

    /* 0 if we're not in a string, else the quote char we're trying to
       match (single or double quote). */
    char quote_char = 0;

    /* If we're inside a string, 1=normal, 3=triple-quoted. */
    int string_type = 0;

    /* Keep track of nesting level for braces/parens/brackets in
       expressions. */
    Py_ssize_t nested_depth = 0;
    char parenstack[MAXLEVEL];

    /* Can only nest one level deep. */
    if (recurse_lvl >= 2) {
        ast_error(c, n, "f-string: expressions nested too deeply");
        return -1;
    }

    /* The first char must be a left brace, or we wouldn't have gotten
       here. Skip over it. */
    assert(**str == '{');
    *str += 1;

    expr_start = *str;
    for (; *str < end; (*str)++) {
        char ch;

        /* Loop invariants. */
        assert(nested_depth >= 0);
        assert(*str >= expr_start && *str < end);
        if (quote_char)
            assert(string_type == 1 || string_type == 3);
        else
            assert(string_type == 0);

        ch = **str;
        /* Nowhere inside an expression is a backslash allowed. */
        if (ch == '\\') {
            /* Error: can't include a backslash character, inside
               parens or strings or not. */
            ast_error(c, n, "f-string expression part "
                            "cannot include a backslash");
            return -1;
        }
        if (quote_char) {
            /* We're inside a string. See if we're at the end. */
            /* This code needs to implement the same non-error logic
               as tok_get from tokenizer.c, at the letter_quote
               label. To actually share that code would be a
               nightmare. But, it's unlikely to change and is small,
               so duplicate it here. Note we don't need to catch all
               of the errors, since they'll be caught when parsing the
               expression. We just need to match the non-error
               cases. Thus we can ignore \n in single-quoted strings,
               for example. Or non-terminated strings. */
            if (ch == quote_char) {
                /* Does this match the string_type (single or triple
                   quoted)? */
                if (string_type == 3) {
                    if (*str+2 < end && *(*str+1) == ch && *(*str+2) == ch) {
                        /* We're at the end of a triple quoted string. */
                        *str += 2;
                        string_type = 0;
                        quote_char = 0;
                        continue;
                    }
                } else {
                    /* We're at the end of a normal string. */
                    quote_char = 0;
                    string_type = 0;
                    continue;
                }
            }
        } else if (ch == '\'' || ch == '"') {
            /* Is this a triple quoted string? */
            if (*str+2 < end && *(*str+1) == ch && *(*str+2) == ch) {
                string_type = 3;
                *str += 2;
            } else {
                /* Start of a normal string. */
                string_type = 1;
            }
            /* Start looking for the end of the string. */
            quote_char = ch;
        } else if (ch == '[' || ch == '{' || ch == '(') {
            if (nested_depth >= MAXLEVEL) {
                ast_error(c, n, "f-string: too many nested parenthesis");
                return -1;
            }
            parenstack[nested_depth] = ch;
            nested_depth++;
        } else if (ch == '#') {
            /* Error: can't include a comment character, inside parens
               or not. */
            ast_error(c, n, "f-string expression part cannot include '#'");
            return -1;
        } else if (nested_depth == 0 &&
                   (ch == '!' || ch == ':' || ch == '}')) {
            /* First, test for the special case of "!=". Since '=' is
               not an allowed conversion character, nothing is lost in
               this test. */
            if (ch == '!' && *str+1 < end && *(*str+1) == '=') {
                /* This isn't a conversion character, just continue. */
                continue;
            }
            /* Normal way out of this loop. */
            break;
        } else if (ch == ']' || ch == '}' || ch == ')') {
            if (!nested_depth) {
                ast_error(c, n, "f-string: unmatched '%c'", ch);
                return -1;
            }
            nested_depth--;
            int opening = parenstack[nested_depth];
            if (!((opening == '(' && ch == ')') ||
                  (opening == '[' && ch == ']') ||
                  (opening == '{' && ch == '}')))
            {
                ast_error(c, n,
                          "f-string: closing parenthesis '%c' "
                          "does not match opening parenthesis '%c'",
                          ch, opening);
                return -1;
            }
        } else {
            /* Just consume this char and loop around. */
        }
    }
    expr_end = *str;
    /* If we leave this loop in a string or with mismatched parens, we
       don't care. We'll get a syntax error when compiling the
       expression. But, we can produce a better error message, so
       let's just do that.*/
    if (quote_char) {
        ast_error(c, n, "f-string: unterminated string");
        return -1;
    }
    if (nested_depth) {
        int opening = parenstack[nested_depth - 1];
        ast_error(c, n, "f-string: unmatched '%c'", opening);
        return -1;
    }

    if (*str >= end)
        goto unexpected_end_of_string;

    /* Compile the expression as soon as possible, so we show errors
       related to the expression before errors related to the
       conversion or format_spec. */
    simple_expression = fstring_compile_expr(expr_start, expr_end, c, n);
    if (!simple_expression)
        return -1;

    /* Check for a conversion char, if present. */
    if (**str == '!') {
        *str += 1;
        if (*str >= end)
            goto unexpected_end_of_string;

        conversion = **str;
        *str += 1;

        /* Validate the conversion. */
        if (!(conversion == 's' || conversion == 'r'
              || conversion == 'a')) {
            ast_error(c, n, "f-string: invalid conversion character: "
                            "expected 's', 'r', or 'a'");
            return -1;
        }
    }

    /* Check for the format spec, if present. */
    if (*str >= end)
        goto unexpected_end_of_string;
    if (**str == ':') {
        *str += 1;
        if (*str >= end)
            goto unexpected_end_of_string;

        /* Parse the format spec. */
        format_spec = fstring_parse(str, end, raw, recurse_lvl+1, c, n);
        if (!format_spec)
            return -1;
    }

    if (*str >= end || **str != '}')
        goto unexpected_end_of_string;

    /* We're at a right brace. Consume it. */
    assert(*str < end);
    assert(**str == '}');
    *str += 1;

    /* And now create the FormattedValue node that represents this
       entire expression with the conversion and format spec. */
    *expression = FormattedValue(simple_expression, conversion,
                                 format_spec, LINENO(n), n->n_col_offset,
                                 n->n_end_lineno, n->n_end_col_offset,
                                 c->c_arena);
    if (!*expression)
        return -1;

    return 0;

unexpected_end_of_string:
    ast_error(c, n, "f-string: expecting '}'");
    return -1;
}
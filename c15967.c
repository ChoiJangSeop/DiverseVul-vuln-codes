static json_t *parse_value(lex_t *lex, size_t flags, json_error_t *error)
{
    json_t *json;

    switch(lex->token) {
        case TOKEN_STRING: {
            const char *value = lex->value.string.val;
            size_t len = lex->value.string.len;

            if(!(flags & JSON_ALLOW_NUL)) {
                if(memchr(value, '\0', len)) {
                    error_set(error, lex, "\\u0000 is not allowed without JSON_ALLOW_NUL");
                    return NULL;
                }
            }

            json = jsonp_stringn_nocheck_own(value, len);
            if(json) {
                lex->value.string.val = NULL;
                lex->value.string.len = 0;
            }
            break;
        }

        case TOKEN_INTEGER: {
            json = json_integer(lex->value.integer);
            break;
        }

        case TOKEN_REAL: {
            json = json_real(lex->value.real);
            break;
        }

        case TOKEN_TRUE:
            json = json_true();
            break;

        case TOKEN_FALSE:
            json = json_false();
            break;

        case TOKEN_NULL:
            json = json_null();
            break;

        case '{':
            json = parse_object(lex, flags, error);
            break;

        case '[':
            json = parse_array(lex, flags, error);
            break;

        case TOKEN_INVALID:
            error_set(error, lex, "invalid token");
            return NULL;

        default:
            error_set(error, lex, "unexpected token");
            return NULL;
    }

    if(!json)
        return NULL;

    return json;
}
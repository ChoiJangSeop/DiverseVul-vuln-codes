_gnutls_asn1_get_structure_xml(ASN1_TYPE structure,
			       gnutls_datum_t * res, int detail)
{
    node_asn *p, *root;
    int k, indent = 0, len, len2, len3;
    opaque tmp[1024];
    char nname[256];
    int ret;
    gnutls_string str;

    if (res == NULL || structure == NULL) {
	gnutls_assert();
	return GNUTLS_E_INVALID_REQUEST;
    }

    _gnutls_string_init(&str, malloc, realloc, free);

    STR_APPEND(XML_HEADER);
    indent = 1;

    root = _asn1_find_node(structure, "");

    if (root == NULL) {
	gnutls_assert();
	_gnutls_string_clear(&str);
	return GNUTLS_E_INTERNAL_ERROR;
    }

    if (detail == GNUTLS_XML_SHOW_ALL)
	ret = asn1_expand_any_defined_by(_gnutls_get_pkix(), &structure);
    /* we don't need to check the error value
     * here.
     */

    if (detail == GNUTLS_XML_SHOW_ALL) {
	ret = _gnutls_x509_expand_extensions(&structure);
	if (ret < 0) {
	    gnutls_assert();
	    return ret;
	}
    }

    p = root;
    while (p) {
	if (is_node_printable(p)) {
	    for (k = 0; k < indent; k++)
		APPEND(" ", 1);

	    if ((ret = normalize_name(p, nname, sizeof(nname))) < 0) {
		_gnutls_string_clear(&str);
		gnutls_assert();
		return ret;
	    }

	    APPEND("<", 1);
	    STR_APPEND(nname);
	}

	if (is_node_printable(p)) {
	    switch (type_field(p->type)) {
	    case TYPE_DEFAULT:
		STR_APPEND(" type=\"DEFAULT\"");
		break;
	    case TYPE_NULL:
		STR_APPEND(" type=\"NULL\"");
		break;
	    case TYPE_IDENTIFIER:
		STR_APPEND(" type=\"IDENTIFIER\"");
		break;
	    case TYPE_INTEGER:
		STR_APPEND(" type=\"INTEGER\"");
		STR_APPEND(" encoding=\"HEX\"");
		break;
	    case TYPE_ENUMERATED:
		STR_APPEND(" type=\"ENUMERATED\"");
		STR_APPEND(" encoding=\"HEX\"");
		break;
	    case TYPE_TIME:
		STR_APPEND(" type=\"TIME\"");
		break;
	    case TYPE_BOOLEAN:
		STR_APPEND(" type=\"BOOLEAN\"");
		break;
	    case TYPE_SEQUENCE:
		STR_APPEND(" type=\"SEQUENCE\"");
		break;
	    case TYPE_BIT_STRING:
		STR_APPEND(" type=\"BIT STRING\"");
		STR_APPEND(" encoding=\"HEX\"");
		break;
	    case TYPE_OCTET_STRING:
		STR_APPEND(" type=\"OCTET STRING\"");
		STR_APPEND(" encoding=\"HEX\"");
		break;
	    case TYPE_SEQUENCE_OF:
		STR_APPEND(" type=\"SEQUENCE OF\"");
		break;
	    case TYPE_OBJECT_ID:
		STR_APPEND(" type=\"OBJECT ID\"");
		break;
	    case TYPE_ANY:
		STR_APPEND(" type=\"ANY\"");
		if (!p->down)
		    STR_APPEND(" encoding=\"HEX\"");
		break;
	    case TYPE_CONSTANT:{
		    ASN1_TYPE up = _asn1_find_up(p);

		    if (up && type_field(up->type) == TYPE_ANY &&
			up->left && up->left->value &&
			up->type & CONST_DEFINED_BY &&
			type_field(up->left->type) == TYPE_OBJECT_ID) {

			if (_gnutls_x509_oid_data_printable
			    (up->left->value) == 0) {
			    STR_APPEND(" encoding=\"HEX\"");
			}

		    }
		}
		break;
	    case TYPE_SET:
		STR_APPEND(" type=\"SET\"");
		break;
	    case TYPE_SET_OF:
		STR_APPEND(" type=\"SET OF\"");
		break;
	    case TYPE_CHOICE:
		STR_APPEND(" type=\"CHOICE\"");
		break;
	    case TYPE_DEFINITIONS:
		STR_APPEND(" type=\"DEFINITIONS\"");
		break;
	    default:
		break;
	    }
	}


	if (p->type == TYPE_BIT_STRING) {
	    len2 = -1;
	    len = _asn1_get_length_der(p->value, &len2);
	    snprintf(tmp, sizeof(tmp), " length=\"%i\"",
		     (len - 1) * 8 - (p->value[len2]));
	    STR_APPEND(tmp);
	}

	if (is_node_printable(p))
	    STR_APPEND(">");

	if (is_node_printable(p)) {
	    const unsigned char *value;

	    if (p->value == NULL)
		value = find_default_value(p);
	    else
		value = p->value;

	    switch (type_field(p->type)) {

	    case TYPE_DEFAULT:
		if (value)
		    STR_APPEND(value);
		break;
	    case TYPE_IDENTIFIER:
		if (value)
		    STR_APPEND(value);
		break;
	    case TYPE_INTEGER:
		if (value) {
		    len2 = -1;
		    len = _asn1_get_length_der(value, &len2);

		    for (k = 0; k < len; k++) {
			snprintf(tmp, sizeof(tmp),
				 "%02X", (value)[k + len2]);
			STR_APPEND(tmp);
		    }

		}
		break;
	    case TYPE_ENUMERATED:
		if (value) {
		    len2 = -1;
		    len = _asn1_get_length_der(value, &len2);

		    for (k = 0; k < len; k++) {
			snprintf(tmp, sizeof(tmp),
				 "%02X", (value)[k + len2]);
			STR_APPEND(tmp);
		    }
		}
		break;
	    case TYPE_TIME:
		if (value)
		    STR_APPEND(value);
		break;
	    case TYPE_BOOLEAN:
		if (value) {
		    if (value[0] == 'T') {
			STR_APPEND("TRUE");
		    } else if (value[0] == 'F') {
			STR_APPEND("FALSE");
		    }
		}
		break;
	    case TYPE_BIT_STRING:
		if (value) {
		    len2 = -1;
		    len = _asn1_get_length_der(value, &len2);

		    for (k = 1; k < len; k++) {
			snprintf(tmp, sizeof(tmp),
				 "%02X", (value)[k + len2]);
			STR_APPEND(tmp);
		    }
		}
		break;
	    case TYPE_OCTET_STRING:
		if (value) {
		    len2 = -1;
		    len = _asn1_get_length_der(value, &len2);
		    for (k = 0; k < len; k++) {
			snprintf(tmp, sizeof(tmp),
				 "%02X", (value)[k + len2]);
			STR_APPEND(tmp);
		    }
		}
		break;
	    case TYPE_OBJECT_ID:
		if (value)
		    STR_APPEND(value);
		break;
	    case TYPE_ANY:
		if (!p->down) {
		    if (value) {
			len3 = -1;
			len2 = _asn1_get_length_der(value, &len3);
			for (k = 0; k < len2; k++) {
			    snprintf(tmp, sizeof(tmp),
				     "%02X", (value)[k + len3]);
			    STR_APPEND(tmp);
			}
		    }
		}
		break;
	    case TYPE_CONSTANT:{
		    ASN1_TYPE up = _asn1_find_up(p);

		    if (up && type_field(up->type) == TYPE_ANY &&
			up->left && up->left->value &&
			up->type & CONST_DEFINED_BY &&
			type_field(up->left->type) == TYPE_OBJECT_ID) {

			len2 = _asn1_get_length_der(up->value, &len3);

			if (len2 > 0 && strcmp(p->name, "type") == 0) {
			    int len = sizeof(tmp);
			    ret =
				_gnutls_x509_oid_data2string(up->left->
							     value,
							     up->value +
							     len3, len2,
							     tmp, &len);

			    if (ret >= 0) {
				STR_APPEND(tmp);
			    }
			} else {
			    for (k = 0; k < len2; k++) {
				snprintf(tmp, sizeof(tmp),
					 "%02X", (up->value)[k + len3]);
				STR_APPEND(tmp);
			    }

			}
		    } else {
			if (value)
			    STR_APPEND(value);
		    }

		}
		break;
	    case TYPE_SET:
	    case TYPE_SET_OF:
	    case TYPE_CHOICE:
	    case TYPE_DEFINITIONS:
	    case TYPE_SEQUENCE_OF:
	    case TYPE_SEQUENCE:
	    case TYPE_NULL:
		break;
	    default:
		break;
	    }
	}

	if (p->down && is_node_printable(p)) {
	    ASN1_TYPE x;
	    p = p->down;
	    indent += 2;
	    x = p;
	    do {
		if (is_node_printable(x)) {
		    STR_APPEND("\n");
		    break;
		}
		x = x->right;
	    } while (x != NULL);
	} else if (p == root) {
	    if (is_node_printable(p)) {
		if ((ret = normalize_name(p, nname, sizeof(nname))) < 0) {
		    _gnutls_string_clear(&str);
		    gnutls_assert();
		    return ret;
		}

		APPEND("</", 2);
		STR_APPEND(nname);
		APPEND(">\n", 2);
	    }
	    p = NULL;
	    break;
	} else {
	    if (is_node_printable(p)) {
		if ((ret = normalize_name(p, nname, sizeof(nname))) < 0) {
		    _gnutls_string_clear(&str);
		    gnutls_assert();
		    return ret;
		}

		APPEND("</", 2);
		STR_APPEND(nname);
		APPEND(">\n", 2);
	    }
	    if (p->right)
		p = p->right;
	    else {
		while (1) {
		    ASN1_TYPE old_p;

		    old_p = p;

		    p = _asn1_find_up(p);
		    indent -= 2;
		    if (is_node_printable(p)) {
			if (!is_leaf(p))	/* XXX */
			    for (k = 0; k < indent; k++)
				STR_APPEND(" ");

			if ((ret =
			     normalize_name(p, nname,
					    sizeof(nname))) < 0) {
			    _gnutls_string_clear(&str);
			    gnutls_assert();
			    return ret;
			}

			APPEND("</", 2);
			STR_APPEND(nname);
			APPEND(">\n", 2);
		    }
		    if (p == root) {
			p = NULL;
			break;
		    }

		    if (p->right) {
			p = p->right;
			break;
		    }
		}
	    }
	}
    }

    STR_APPEND(XML_FOOTER);
    APPEND("\n\0", 2);

    *res = _gnutls_string2datum(&str);
    res->size -= 1;		/* null is not included in size */

    return 0;
}
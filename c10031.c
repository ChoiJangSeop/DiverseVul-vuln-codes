static sdlSoapBindingFunctionHeaderPtr wsdl_soap_binding_header(sdlCtx* ctx, xmlNodePtr header, char* wsdl_soap_namespace, int fault)
{
	xmlAttrPtr tmp;
	xmlNodePtr message, part;
	char *ctype;
	sdlSoapBindingFunctionHeaderPtr h;

	tmp = get_attribute(header->properties, "message");
	if (!tmp) {
		soap_error0(E_ERROR, "Parsing WSDL: Missing message attribute for <header>");
	}

	ctype = strrchr((char*)tmp->children->content,':');
	if (ctype == NULL) {
		ctype = (char*)tmp->children->content;
	} else {
		++ctype;
	}
	if ((message = zend_hash_str_find_ptr(&ctx->messages, ctype, strlen(ctype))) == NULL) {
		soap_error1(E_ERROR, "Parsing WSDL: Missing <message> with name '%s'", tmp->children->content);
	}

	tmp = get_attribute(header->properties, "part");
	if (!tmp) {
		soap_error0(E_ERROR, "Parsing WSDL: Missing part attribute for <header>");
	}
	part = get_node_with_attribute_ex(message->children, "part", WSDL_NAMESPACE, "name", (char*)tmp->children->content, NULL);
	if (!part) {
		soap_error1(E_ERROR, "Parsing WSDL: Missing part '%s' in <message>", tmp->children->content);
	}

	h = emalloc(sizeof(sdlSoapBindingFunctionHeader));
	memset(h, 0, sizeof(sdlSoapBindingFunctionHeader));
	h->name = estrdup((char*)tmp->children->content);

	tmp = get_attribute(header->properties, "use");
	if (tmp && !strncmp((char*)tmp->children->content, "encoded", sizeof("encoded"))) {
		h->use = SOAP_ENCODED;
	} else {
		h->use = SOAP_LITERAL;
	}

	tmp = get_attribute(header->properties, "namespace");
	if (tmp) {
		h->ns = estrdup((char*)tmp->children->content);
	}

	if (h->use == SOAP_ENCODED) {
		tmp = get_attribute(header->properties, "encodingStyle");
		if (tmp) {
			if (strncmp((char*)tmp->children->content, SOAP_1_1_ENC_NAMESPACE, sizeof(SOAP_1_1_ENC_NAMESPACE)) == 0) {
				h->encodingStyle = SOAP_ENCODING_1_1;
			} else if (strncmp((char*)tmp->children->content, SOAP_1_2_ENC_NAMESPACE, sizeof(SOAP_1_2_ENC_NAMESPACE)) == 0) {
				h->encodingStyle = SOAP_ENCODING_1_2;
			} else {
				soap_error1(E_ERROR, "Parsing WSDL: Unknown encodingStyle '%s'", tmp->children->content);
			}
		} else {
			soap_error0(E_ERROR, "Parsing WSDL: Unspecified encodingStyle");
		}
	}

	tmp = get_attribute(part->properties, "type");
	if (tmp != NULL) {
		h->encode = get_encoder_from_prefix(ctx->sdl, part, tmp->children->content);
	} else {
		tmp = get_attribute(part->properties, "element");
		if (tmp != NULL) {
			h->element = get_element(ctx->sdl, part, tmp->children->content);
			if (h->element) {
				h->encode = h->element->encode;
				if (!h->ns && h->element->namens) {
					h->ns = estrdup(h->element->namens);
				}
				if (h->element->name) {
					efree(h->name);
					h->name = estrdup(h->element->name);
				}
			}
		}
	}
	if (!fault) {
		xmlNodePtr trav = header->children;
		while (trav != NULL) {
			if (node_is_equal_ex(trav, "headerfault", wsdl_soap_namespace)) {
				sdlSoapBindingFunctionHeaderPtr hf = wsdl_soap_binding_header(ctx, trav, wsdl_soap_namespace, 1);
				smart_str key = {0};

				if (h->headerfaults == NULL) {
					h->headerfaults = emalloc(sizeof(HashTable));
					zend_hash_init(h->headerfaults, 0, NULL, delete_header, 0);
				}

				if (hf->ns) {
					smart_str_appends(&key,hf->ns);
					smart_str_appendc(&key,':');
				}
				smart_str_appends(&key,hf->name);
				smart_str_0(&key);
				if (zend_hash_add_ptr(h->headerfaults, key.s, hf) == NULL) {
					delete_header_int(hf);
				}
				smart_str_free(&key);
			} else if (is_wsdl_element(trav) && !node_is_equal(trav,"documentation")) {
				soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav->name);
			}
			trav = trav->next;
		}
	}
	return h;
}
static void wsdl_soap_binding_body(sdlCtx* ctx, xmlNodePtr node, char* wsdl_soap_namespace, sdlSoapBindingFunctionBody *binding, HashTable* params)
{
	xmlNodePtr body, trav;
	xmlAttrPtr tmp;

	trav = node->children;
	while (trav != NULL) {
		if (node_is_equal_ex(trav, "body", wsdl_soap_namespace)) {
			body = trav;

			tmp = get_attribute(body->properties, "use");
			if (tmp && !strncmp((char*)tmp->children->content, "literal", sizeof("literal"))) {
				binding->use = SOAP_LITERAL;
			} else {
				binding->use = SOAP_ENCODED;
			}

			tmp = get_attribute(body->properties, "namespace");
			if (tmp) {
				binding->ns = estrdup((char*)tmp->children->content);
			}

			tmp = get_attribute(body->properties, "parts");
			if (tmp) {
				HashTable    ht;
				char *parts = (char*)tmp->children->content;

				/* Delete all parts those are not in the "parts" attribute */
				zend_hash_init(&ht, 0, NULL, delete_parameter, 0);
				while (*parts) {
					sdlParamPtr param;
					int found = 0;
					char *end;

					while (*parts == ' ') ++parts;
					if (*parts == '\0') break;
					end = strchr(parts, ' ');
					if (end) *end = '\0';
					ZEND_HASH_FOREACH_PTR(params, param) {
						if (param->paramName &&
						    strcmp(parts, param->paramName) == 0) {
					  	sdlParamPtr x_param;
					  	x_param = emalloc(sizeof(sdlParam));
					  	*x_param = *param;
					  	param->paramName = NULL;
					  	zend_hash_next_index_insert_ptr(&ht, x_param);
					  	found = 1;
					  	break;
						}
					} ZEND_HASH_FOREACH_END();
					if (!found) {
						soap_error1(E_ERROR, "Parsing WSDL: Missing part '%s' in <message>", parts);
					}
					parts += strlen(parts);
					if (end) *end = ' ';
				}
				zend_hash_destroy(params);
				*params = ht;
			}

			if (binding->use == SOAP_ENCODED) {
				tmp = get_attribute(body->properties, "encodingStyle");
				if (tmp) {
					if (strncmp((char*)tmp->children->content, SOAP_1_1_ENC_NAMESPACE, sizeof(SOAP_1_1_ENC_NAMESPACE)) == 0) {
						binding->encodingStyle = SOAP_ENCODING_1_1;
					} else if (strncmp((char*)tmp->children->content, SOAP_1_2_ENC_NAMESPACE, sizeof(SOAP_1_2_ENC_NAMESPACE)) == 0) {
						binding->encodingStyle = SOAP_ENCODING_1_2;
					} else {
						soap_error1(E_ERROR, "Parsing WSDL: Unknown encodingStyle '%s'", tmp->children->content);
					}
				} else {
					soap_error0(E_ERROR, "Parsing WSDL: Unspecified encodingStyle");
				}
			}
		} else if (node_is_equal_ex(trav, "header", wsdl_soap_namespace)) {
			sdlSoapBindingFunctionHeaderPtr h = wsdl_soap_binding_header(ctx, trav, wsdl_soap_namespace, 0);
			smart_str key = {0};

			if (binding->headers == NULL) {
				binding->headers = emalloc(sizeof(HashTable));
				zend_hash_init(binding->headers, 0, NULL, delete_header, 0);
			}

			if (h->ns) {
				smart_str_appends(&key,h->ns);
				smart_str_appendc(&key,':');
			}
			smart_str_appends(&key,h->name);
			smart_str_0(&key);
			if (zend_hash_add_ptr(binding->headers, key.s, h) == NULL) {
				delete_header_int(h);
			}
			smart_str_free(&key);
		} else if (is_wsdl_element(trav) && !node_is_equal(trav,"documentation")) {
			soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav->name);
		}
		trav = trav->next;
	}
}
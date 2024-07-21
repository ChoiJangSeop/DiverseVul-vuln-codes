static HashTable* wsdl_message(sdlCtx *ctx, xmlChar* message_name)
{
	xmlNodePtr trav, part, message = NULL, tmp;
	HashTable* parameters = NULL;
	char *ctype;

	ctype = strrchr((char*)message_name,':');
	if (ctype == NULL) {
		ctype = (char*)message_name;
	} else {
		++ctype;
	}
	if ((tmp = zend_hash_str_find_ptr(&ctx->messages, ctype, strlen(ctype))) == NULL) {
		soap_error1(E_ERROR, "Parsing WSDL: Missing <message> with name '%s'", message_name);
	}
	message = tmp;

	parameters = emalloc(sizeof(HashTable));
	zend_hash_init(parameters, 0, NULL, delete_parameter, 0);

	trav = message->children;
	while (trav != NULL) {
		xmlAttrPtr element, type, name;
		sdlParamPtr param;

		if (trav->ns != NULL && strcmp((char*)trav->ns->href, WSDL_NAMESPACE) != 0) {
			soap_error1(E_ERROR, "Parsing WSDL: Unexpected extensibility element <%s>", trav->name);
		}
		if (node_is_equal(trav,"documentation")) {
			trav = trav->next;
			continue;
		}
		if (!node_is_equal(trav,"part")) {
			soap_error1(E_ERROR, "Parsing WSDL: Unexpected WSDL element <%s>", trav->name);
		}
		part = trav;
		param = emalloc(sizeof(sdlParam));
		memset(param,0,sizeof(sdlParam));
		param->order = 0;

		name = get_attribute(part->properties, "name");
		if (name == NULL) {
			soap_error1(E_ERROR, "Parsing WSDL: No name associated with <part> '%s'", message->name);
		}

		param->paramName = estrdup((char*)name->children->content);

		type = get_attribute(part->properties, "type");
		if (type != NULL) {
			param->encode = get_encoder_from_prefix(ctx->sdl, part, type->children->content);
		} else {
			element = get_attribute(part->properties, "element");
			if (element != NULL) {
				param->element = get_element(ctx->sdl, part, element->children->content);
				if (param->element) {
					param->encode = param->element->encode;
				}
			}
		}

		zend_hash_next_index_insert_ptr(parameters, param);

		trav = trav->next;
	}
	return parameters;
}
xmlSchemaFormatItemForReport(xmlChar **buf,
		     const xmlChar *itemDes,
		     xmlSchemaBasicItemPtr item,
		     xmlNodePtr itemNode)
{
    xmlChar *str = NULL;
    int named = 1;

    if (*buf != NULL) {
	xmlFree(*buf);
	*buf = NULL;
    }

    if (itemDes != NULL) {
	*buf = xmlStrdup(itemDes);
    } else if (item != NULL) {
	switch (item->type) {
	case XML_SCHEMA_TYPE_BASIC: {
	    xmlSchemaTypePtr type = WXS_TYPE_CAST item;

	    if (WXS_IS_ATOMIC(type))
		*buf = xmlStrdup(BAD_CAST "atomic type 'xs:");
	    else if (WXS_IS_LIST(type))
		*buf = xmlStrdup(BAD_CAST "list type 'xs:");
	    else if (WXS_IS_UNION(type))
		*buf = xmlStrdup(BAD_CAST "union type 'xs:");
	    else
		*buf = xmlStrdup(BAD_CAST "simple type 'xs:");
	    *buf = xmlStrcat(*buf, type->name);
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    }
	    break;
	case XML_SCHEMA_TYPE_SIMPLE: {
	    xmlSchemaTypePtr type = WXS_TYPE_CAST item;

	    if (type->flags & XML_SCHEMAS_TYPE_GLOBAL) {
		*buf = xmlStrdup(BAD_CAST"");
	    } else {
		*buf = xmlStrdup(BAD_CAST "local ");
	    }
	    if (WXS_IS_ATOMIC(type))
		*buf = xmlStrcat(*buf, BAD_CAST "atomic type");
	    else if (WXS_IS_LIST(type))
		*buf = xmlStrcat(*buf, BAD_CAST "list type");
	    else if (WXS_IS_UNION(type))
		*buf = xmlStrcat(*buf, BAD_CAST "union type");
	    else
		*buf = xmlStrcat(*buf, BAD_CAST "simple type");
	    if (type->flags & XML_SCHEMAS_TYPE_GLOBAL) {
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, type->name);
		*buf = xmlStrcat(*buf, BAD_CAST "'");
	    }
	    }
	    break;
	case XML_SCHEMA_TYPE_COMPLEX: {
	    xmlSchemaTypePtr type = WXS_TYPE_CAST item;

	    if (type->flags & XML_SCHEMAS_TYPE_GLOBAL)
		*buf = xmlStrdup(BAD_CAST "");
	    else
		*buf = xmlStrdup(BAD_CAST "local ");
	    *buf = xmlStrcat(*buf, BAD_CAST "complex type");
	    if (type->flags & XML_SCHEMAS_TYPE_GLOBAL) {
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, type->name);
		*buf = xmlStrcat(*buf, BAD_CAST "'");
	    }
	    }
	    break;
	case XML_SCHEMA_TYPE_ATTRIBUTE_USE: {
		xmlSchemaAttributeUsePtr ause;

		ause = WXS_ATTR_USE_CAST item;
		*buf = xmlStrdup(BAD_CAST "attribute use ");
		if (WXS_ATTRUSE_DECL(ause) != NULL) {
		    *buf = xmlStrcat(*buf, BAD_CAST "'");
		    *buf = xmlStrcat(*buf,
			xmlSchemaGetComponentQName(&str, WXS_ATTRUSE_DECL(ause)));
		    FREE_AND_NULL(str)
			*buf = xmlStrcat(*buf, BAD_CAST "'");
		} else {
		    *buf = xmlStrcat(*buf, BAD_CAST "(unknown)");
		}
	    }
	    break;
	case XML_SCHEMA_TYPE_ATTRIBUTE: {
		xmlSchemaAttributePtr attr;

		attr = (xmlSchemaAttributePtr) item;
		*buf = xmlStrdup(BAD_CAST "attribute decl.");
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
		    attr->targetNamespace, attr->name));
		FREE_AND_NULL(str)
		    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    }
	    break;
	case XML_SCHEMA_TYPE_ATTRIBUTEGROUP:
	    xmlSchemaGetComponentDesignation(buf, item);
	    break;
	case XML_SCHEMA_TYPE_ELEMENT: {
		xmlSchemaElementPtr elem;

		elem = (xmlSchemaElementPtr) item;
		*buf = xmlStrdup(BAD_CAST "element decl.");
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
		    elem->targetNamespace, elem->name));
		*buf = xmlStrcat(*buf, BAD_CAST "'");
	    }
	    break;
	case XML_SCHEMA_TYPE_IDC_UNIQUE:
	case XML_SCHEMA_TYPE_IDC_KEY:
	case XML_SCHEMA_TYPE_IDC_KEYREF:
	    if (item->type == XML_SCHEMA_TYPE_IDC_UNIQUE)
		*buf = xmlStrdup(BAD_CAST "unique '");
	    else if (item->type == XML_SCHEMA_TYPE_IDC_KEY)
		*buf = xmlStrdup(BAD_CAST "key '");
	    else
		*buf = xmlStrdup(BAD_CAST "keyRef '");
	    *buf = xmlStrcat(*buf, ((xmlSchemaIDCPtr) item)->name);
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    break;
	case XML_SCHEMA_TYPE_ANY:
	case XML_SCHEMA_TYPE_ANY_ATTRIBUTE:
	    *buf = xmlStrdup(xmlSchemaWildcardPCToString(
		    ((xmlSchemaWildcardPtr) item)->processContents));
	    *buf = xmlStrcat(*buf, BAD_CAST " wildcard");
	    break;
	case XML_SCHEMA_FACET_MININCLUSIVE:
	case XML_SCHEMA_FACET_MINEXCLUSIVE:
	case XML_SCHEMA_FACET_MAXINCLUSIVE:
	case XML_SCHEMA_FACET_MAXEXCLUSIVE:
	case XML_SCHEMA_FACET_TOTALDIGITS:
	case XML_SCHEMA_FACET_FRACTIONDIGITS:
	case XML_SCHEMA_FACET_PATTERN:
	case XML_SCHEMA_FACET_ENUMERATION:
	case XML_SCHEMA_FACET_WHITESPACE:
	case XML_SCHEMA_FACET_LENGTH:
	case XML_SCHEMA_FACET_MAXLENGTH:
	case XML_SCHEMA_FACET_MINLENGTH:
	    *buf = xmlStrdup(BAD_CAST "facet '");
	    *buf = xmlStrcat(*buf, xmlSchemaFacetTypeToString(item->type));
	    *buf = xmlStrcat(*buf, BAD_CAST "'");
	    break;
	case XML_SCHEMA_TYPE_GROUP: {
		*buf = xmlStrdup(BAD_CAST "model group def.");
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, xmlSchemaGetComponentQName(&str, item));
		*buf = xmlStrcat(*buf, BAD_CAST "'");
		FREE_AND_NULL(str)
	    }
	    break;
	case XML_SCHEMA_TYPE_SEQUENCE:
	case XML_SCHEMA_TYPE_CHOICE:
	case XML_SCHEMA_TYPE_ALL:
	case XML_SCHEMA_TYPE_PARTICLE:
	    *buf = xmlStrdup(WXS_ITEM_TYPE_NAME(item));
	    break;
	case XML_SCHEMA_TYPE_NOTATION: {
		*buf = xmlStrdup(WXS_ITEM_TYPE_NAME(item));
		*buf = xmlStrcat(*buf, BAD_CAST " '");
		*buf = xmlStrcat(*buf, xmlSchemaGetComponentQName(&str, item));
		*buf = xmlStrcat(*buf, BAD_CAST "'");
		FREE_AND_NULL(str);
	    }
	default:
	    named = 0;
	}
    } else
	named = 0;

    if ((named == 0) && (itemNode != NULL)) {
	xmlNodePtr elem;

	if (itemNode->type == XML_ATTRIBUTE_NODE)
	    elem = itemNode->parent;
	else
	    elem = itemNode;
	*buf = xmlStrdup(BAD_CAST "Element '");
	if (elem->ns != NULL) {
	    *buf = xmlStrcat(*buf,
		xmlSchemaFormatQName(&str, elem->ns->href, elem->name));
	    FREE_AND_NULL(str)
	} else
	    *buf = xmlStrcat(*buf, elem->name);
	*buf = xmlStrcat(*buf, BAD_CAST "'");

    }
    if ((itemNode != NULL) && (itemNode->type == XML_ATTRIBUTE_NODE)) {
	*buf = xmlStrcat(*buf, BAD_CAST ", attribute '");
	if (itemNode->ns != NULL) {
	    *buf = xmlStrcat(*buf, xmlSchemaFormatQName(&str,
		itemNode->ns->href, itemNode->name));
	    FREE_AND_NULL(str)
	} else
	    *buf = xmlStrcat(*buf, itemNode->name);
	*buf = xmlStrcat(*buf, BAD_CAST "'");
    }
    FREE_AND_NULL(str)

    return (*buf);
}
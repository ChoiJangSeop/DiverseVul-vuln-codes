int attr_is_equal_ex(xmlAttrPtr node, char *name, char *ns)
{
	if (name == NULL || strcmp((char*)node->name, name) == 0) {
		if (ns) {
			xmlNsPtr nsPtr = attr_find_ns(node);
			if (nsPtr) {
				return (strcmp((char*)nsPtr->href, ns) == 0);
			} else {
				return FALSE;
			}
		}
		return TRUE;
	}
	return FALSE;
}
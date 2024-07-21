_XimGetAttributeID(
    Xim			  im,
    CARD16		 *buf)
{
    unsigned int	  n;
    XIMResourceList	  res;
    char		 *names;
    int			  names_len;
    XPointer		  tmp;
    XIMValuesList	 *values_list;
    char		**values;
    int			  values_len;
    register int	  i;
    INT16		  len;
    INT16		  min_len = sizeof(CARD16) /* sizeof attribute ID */
				  + sizeof(CARD16) /* sizeof type of value */
				  + sizeof(INT16); /* sizeof length of attr */
    /*
     * IM attribute ID
     */

    if (!(n = _XimCountNumberOfAttr(buf[0], &buf[1], &names_len)))
	return False;

    if (!(res = Xcalloc(n, sizeof(XIMResource))))
	return False;

    values_len = sizeof(XIMValuesList) + (sizeof(char **) * n) + names_len;
    if (!(tmp = Xcalloc(1, values_len))) {
	Xfree(res);
	return False;
    }

    values_list = (XIMValuesList *)tmp;
    values = (char **)((char *)tmp + sizeof(XIMValuesList));
    names = (char *)((char *)values + (sizeof(char **) * n));

    values_list->count_values = n;
    values_list->supported_values = values;

    buf++;
    for (i = 0; i < n; i++) {
	len = buf[2];
	(void)memcpy(names, (char *)&buf[3], len);
	values[i] = names;
	names[len] = '\0';
	res[i].resource_name = names;
	res[i].resource_size = buf[1];
	res[i].id	     = buf[0];
	names += (len + 1);
	len += (min_len + XIM_PAD(len + 2));
	buf = (CARD16 *)((char *)buf + len);
    }
    _XIMCompileResourceList(res, n);

    Xfree(im->core.im_resources);
    Xfree(im->core.im_values_list);

    im->core.im_resources     = res;
    im->core.im_num_resources = n;
    im->core.im_values_list   = values_list;

    /*
     * IC attribute ID
     */

    if (!(n = _XimCountNumberOfAttr(buf[0], &buf[2], &names_len)))
	return False;

    if (!(res = Xcalloc(n, sizeof(XIMResource))))
	return False;

    values_len = sizeof(XIMValuesList) + (sizeof(char **) * n) + names_len;
    if (!(tmp = Xcalloc(1, values_len))) {
	Xfree(res);
	return False;
    }

    values_list = (XIMValuesList *)tmp;
    values = (char **)((char *)tmp + sizeof(XIMValuesList));
    names = (char *)((char *)values + (sizeof(char **) * n));

    values_list->count_values = n;
    values_list->supported_values = values;

    buf += 2;
    for (i = 0; i < n; i++) {
	len = buf[2];
	(void)memcpy(names, (char *)&buf[3], len);
	values[i] = names;
	names[len] = '\0';
	res[i].resource_name = names;
	res[i].resource_size = buf[1];
	res[i].id	     = buf[0];
	names += (len + 1);
	len += (min_len + XIM_PAD(len + 2));
	buf = (CARD16 *)((char *)buf + len);
    }
    _XIMCompileResourceList(res, n);


    Xfree(im->core.ic_resources);
    Xfree(im->core.ic_values_list);

    im->core.ic_resources     = res;
    im->core.ic_num_resources = n;
    im->core.ic_values_list   = values_list;

    return True;
}
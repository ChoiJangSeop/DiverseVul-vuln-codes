ASN1_OBJECT *c2i_ASN1_OBJECT(ASN1_OBJECT **a, const unsigned char **pp,
	     long len)
	{
	ASN1_OBJECT *ret=NULL;
	const unsigned char *p;
	unsigned char *data;
	int i;
	/* Sanity check OID encoding: can't have leading 0x80 in
	 * subidentifiers, see: X.690 8.19.2
	 */
	for (i = 0, p = *pp; i < len; i++, p++)
		{
		if (*p == 0x80 && (!i || !(p[-1] & 0x80)))
			{
			ASN1err(ASN1_F_C2I_ASN1_OBJECT,ASN1_R_INVALID_OBJECT_ENCODING);
			return NULL;
			}
		}

	/* only the ASN1_OBJECTs from the 'table' will have values
	 * for ->sn or ->ln */
	if ((a == NULL) || ((*a) == NULL) ||
		!((*a)->flags & ASN1_OBJECT_FLAG_DYNAMIC))
		{
		if ((ret=ASN1_OBJECT_new()) == NULL) return(NULL);
		}
	else	ret=(*a);

	p= *pp;
	/* detach data from object */
	data = (unsigned char *)ret->data;
	ret->data = NULL;
	/* once detached we can change it */
	if ((data == NULL) || (ret->length < len))
		{
		ret->length=0;
		if (data != NULL) OPENSSL_free(data);
		data=(unsigned char *)OPENSSL_malloc(len ? (int)len : 1);
		if (data == NULL)
			{ i=ERR_R_MALLOC_FAILURE; goto err; }
		ret->flags|=ASN1_OBJECT_FLAG_DYNAMIC_DATA;
		}
	memcpy(data,p,(int)len);
	/* reattach data to object, after which it remains const */
	ret->data  =data;
	ret->length=(int)len;
	ret->sn=NULL;
	ret->ln=NULL;
	/* ret->flags=ASN1_OBJECT_FLAG_DYNAMIC; we know it is dynamic */
	p+=len;

	if (a != NULL) (*a)=ret;
	*pp=p;
	return(ret);
err:
	ASN1err(ASN1_F_C2I_ASN1_OBJECT,i);
	if ((ret != NULL) && ((a == NULL) || (*a != ret)))
		ASN1_OBJECT_free(ret);
	return(NULL);
	}
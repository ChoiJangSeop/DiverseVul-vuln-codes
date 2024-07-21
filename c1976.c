_asn1_extract_tag_der (asn1_node node, const unsigned char *der, int der_len,
		       int *ret_len)
{
  asn1_node p;
  int counter, len2, len3, is_tag_implicit;
  unsigned long tag, tag_implicit = 0;
  unsigned char class, class2, class_implicit = 0;

  if (der_len <= 0)
    return ASN1_GENERIC_ERROR;

  counter = is_tag_implicit = 0;

  if (node->type & CONST_TAG)
    {
      p = node->down;
      while (p)
	{
	  if (type_field (p->type) == ASN1_ETYPE_TAG)
	    {
	      if (p->type & CONST_APPLICATION)
		class2 = ASN1_CLASS_APPLICATION;
	      else if (p->type & CONST_UNIVERSAL)
		class2 = ASN1_CLASS_UNIVERSAL;
	      else if (p->type & CONST_PRIVATE)
		class2 = ASN1_CLASS_PRIVATE;
	      else
		class2 = ASN1_CLASS_CONTEXT_SPECIFIC;

	      if (p->type & CONST_EXPLICIT)
		{
		  if (asn1_get_tag_der
		      (der + counter, der_len - counter, &class, &len2,
		       &tag) != ASN1_SUCCESS)
		    return ASN1_DER_ERROR;

		  if (counter + len2 > der_len)
		    return ASN1_DER_ERROR;
		  counter += len2;

		  len3 =
		    asn1_get_length_ber (der + counter, der_len - counter,
					 &len2);
		  if (len3 < 0)
		    return ASN1_DER_ERROR;

		  counter += len2;
		  if (counter > der_len)
		    return ASN1_DER_ERROR;

		  if (!is_tag_implicit)
		    {
		      if ((class != (class2 | ASN1_CLASS_STRUCTURED)) ||
			  (tag != strtoul ((char *) p->value, NULL, 10)))
			return ASN1_TAG_ERROR;
		    }
		  else
		    {		/* ASN1_TAG_IMPLICIT */
		      if ((class != class_implicit) || (tag != tag_implicit))
			return ASN1_TAG_ERROR;
		    }
		  is_tag_implicit = 0;
		}
	      else
		{		/* ASN1_TAG_IMPLICIT */
		  if (!is_tag_implicit)
		    {
		      if ((type_field (node->type) == ASN1_ETYPE_SEQUENCE) ||
			  (type_field (node->type) == ASN1_ETYPE_SEQUENCE_OF)
			  || (type_field (node->type) == ASN1_ETYPE_SET)
			  || (type_field (node->type) == ASN1_ETYPE_SET_OF))
			class2 |= ASN1_CLASS_STRUCTURED;
		      class_implicit = class2;
		      tag_implicit = strtoul ((char *) p->value, NULL, 10);
		      is_tag_implicit = 1;
		    }
		}
	    }
	  p = p->right;
	}
    }

  if (is_tag_implicit)
    {
      if (asn1_get_tag_der
	  (der + counter, der_len - counter, &class, &len2,
	   &tag) != ASN1_SUCCESS)
	return ASN1_DER_ERROR;
      if (counter + len2 > der_len)
	return ASN1_DER_ERROR;

      if ((class != class_implicit) || (tag != tag_implicit))
	{
	  if (type_field (node->type) == ASN1_ETYPE_OCTET_STRING)
	    {
	      class_implicit |= ASN1_CLASS_STRUCTURED;
	      if ((class != class_implicit) || (tag != tag_implicit))
		return ASN1_TAG_ERROR;
	    }
	  else
	    return ASN1_TAG_ERROR;
	}
    }
  else
    {
      unsigned type = type_field (node->type);
      if (type == ASN1_ETYPE_TAG)
	{
	  counter = 0;
	  *ret_len = counter;
	  return ASN1_SUCCESS;
	}

      if (asn1_get_tag_der
	  (der + counter, der_len - counter, &class, &len2,
	   &tag) != ASN1_SUCCESS)
	return ASN1_DER_ERROR;

      if (counter + len2 > der_len)
	return ASN1_DER_ERROR;

      switch (type)
	{
	case ASN1_ETYPE_NULL:
	case ASN1_ETYPE_BOOLEAN:
	case ASN1_ETYPE_INTEGER:
	case ASN1_ETYPE_ENUMERATED:
	case ASN1_ETYPE_OBJECT_ID:
	case ASN1_ETYPE_GENERALSTRING:
	case ASN1_ETYPE_NUMERIC_STRING:
	case ASN1_ETYPE_IA5_STRING:
	case ASN1_ETYPE_TELETEX_STRING:
	case ASN1_ETYPE_PRINTABLE_STRING:
	case ASN1_ETYPE_UNIVERSAL_STRING:
	case ASN1_ETYPE_BMP_STRING:
	case ASN1_ETYPE_UTF8_STRING:
	case ASN1_ETYPE_VISIBLE_STRING:
	case ASN1_ETYPE_BIT_STRING:
	case ASN1_ETYPE_SEQUENCE:
	case ASN1_ETYPE_SEQUENCE_OF:
	case ASN1_ETYPE_SET:
	case ASN1_ETYPE_SET_OF:
	case ASN1_ETYPE_GENERALIZED_TIME:
	case ASN1_ETYPE_UTC_TIME:
	  if ((class != _asn1_tags[type].class)
	      || (tag != _asn1_tags[type].tag))
	    return ASN1_DER_ERROR;
	  break;

	case ASN1_ETYPE_OCTET_STRING:
	  /* OCTET STRING is handled differently to allow
	   * BER encodings (structured class). */
	  if (((class != ASN1_CLASS_UNIVERSAL)
	       && (class != (ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED)))
	      || (tag != ASN1_TAG_OCTET_STRING))
	    return ASN1_DER_ERROR;
	  break;
	case ASN1_ETYPE_ANY:
	  counter -= len2;
	  break;
	case ASN1_ETYPE_CHOICE:
	  counter -= len2;
	  break;
	default:
	  return ASN1_DER_ERROR;
	  break;
	}
    }

  counter += len2;
  *ret_len = counter;
  return ASN1_SUCCESS;
}
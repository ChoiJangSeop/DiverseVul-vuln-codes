asn1_der_decoding (asn1_node * element, const void *ider, int len,
		   char *errorDescription)
{
  asn1_node node, p, p2, p3;
  char temp[128];
  int counter, len2, len3, len4, move, ris, tlen;
  unsigned char class;
  unsigned long tag;
  int indefinite, result;
  const unsigned char *der = ider;

  node = *element;

  if (errorDescription != NULL)
    errorDescription[0] = 0;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  if (node->type & CONST_OPTION)
    {
      result = ASN1_GENERIC_ERROR;
      goto cleanup;
    }

  counter = 0;
  move = DOWN;
  p = node;
  while (1)
    {
      ris = ASN1_SUCCESS;
      if (move != UP)
	{
	  if (p->type & CONST_SET)
	    {
	      p2 = _asn1_find_up (p);
	      len2 = _asn1_strtol (p2->value, NULL, 10);
	      if (len2 == -1)
		{
		  if (!der[counter] && !der[counter + 1])
		    {
		      p = p2;
		      move = UP;
		      counter += 2;
		      continue;
		    }
		}
	      else if (counter == len2)
		{
		  p = p2;
		  move = UP;
		  continue;
		}
	      else if (counter > len2)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}
	      p2 = p2->down;
	      while (p2)
		{
		  if ((p2->type & CONST_SET) && (p2->type & CONST_NOT_USED))
		    {
		      ris =
			  extract_tag_der_recursive (p2, der + counter,
						 len - counter, &len2);
		      if (ris == ASN1_SUCCESS)
			{
			  p2->type &= ~CONST_NOT_USED;
			  p = p2;
			  break;
			}
		    }
		  p2 = p2->right;
		}
	      if (p2 == NULL)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}
	    }

	  if ((p->type & CONST_OPTION) || (p->type & CONST_DEFAULT))
	    {
	      p2 = _asn1_find_up (p);
	      len2 = _asn1_strtol (p2->value, NULL, 10);
	      if (counter == len2)
		{
		  if (p->right)
		    {
		      p2 = p->right;
		      move = RIGHT;
		    }
		  else
		    move = UP;

		  if (p->type & CONST_OPTION)
		    asn1_delete_structure (&p);

		  p = p2;
		  continue;
		}
	    }

	  if (type_field (p->type) == ASN1_ETYPE_CHOICE)
	    {
	      while (p->down)
		{
		  if (counter < len)
		    ris =
		      extract_tag_der_recursive (p->down, der + counter,
					     len - counter, &len2);
		  else
		    ris = ASN1_DER_ERROR;
		  if (ris == ASN1_SUCCESS)
		    {
		      delete_unneeded_choice_fields(p->down);
		      break;
		    }
		  else if (ris == ASN1_ERROR_TYPE_ANY)
		    {
		      result = ASN1_ERROR_TYPE_ANY;
		      goto cleanup;
		    }
		  else
		    {
		      p2 = p->down;
		      asn1_delete_structure (&p2);
		    }
		}

	      if (p->down == NULL)
		{
		  if (!(p->type & CONST_OPTION))
		    {
		      result = ASN1_DER_ERROR;
		      goto cleanup;
		    }
		}
	      else if (type_field (p->type) != ASN1_ETYPE_CHOICE)
		p = p->down;
	    }

	  if ((p->type & CONST_OPTION) || (p->type & CONST_DEFAULT))
	    {
	      p2 = _asn1_find_up (p);
	      len2 = _asn1_strtol (p2->value, NULL, 10);
	      if ((len2 != -1) && (counter > len2))
		ris = ASN1_TAG_ERROR;
	    }

	  if (ris == ASN1_SUCCESS)
	    ris =
	      extract_tag_der_recursive (p, der + counter, len - counter, &len2);
	  if (ris != ASN1_SUCCESS)
	    {
	      if (p->type & CONST_OPTION)
		{
		  p->type |= CONST_NOT_USED;
		  move = RIGHT;
		}
	      else if (p->type & CONST_DEFAULT)
		{
		  _asn1_set_value (p, NULL, 0);
		  move = RIGHT;
		}
	      else
		{
		  if (errorDescription != NULL)
		    _asn1_error_description_tag_error (p, errorDescription);

		  result = ASN1_TAG_ERROR;
		  goto cleanup;
		}
	    }
	  else
	    counter += len2;
	}

      if (ris == ASN1_SUCCESS)
	{
	  switch (type_field (p->type))
	    {
	    case ASN1_ETYPE_NULL:
	      if (der[counter])
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}
	      counter++;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_BOOLEAN:
	      if (der[counter++] != 1)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}
	      if (der[counter++] == 0)
		_asn1_set_value (p, "F", 1);
	      else
		_asn1_set_value (p, "T", 1);
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_INTEGER:
	    case ASN1_ETYPE_ENUMERATED:
	      len2 =
		asn1_get_length_der (der + counter, len - counter, &len3);
	      if (len2 < 0)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}

	      _asn1_set_value (p, der + counter, len3 + len2);
	      counter += len3 + len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_OBJECT_ID:
	      result =
		_asn1_get_objectid_der (der + counter, len - counter, &len2,
					temp, sizeof (temp));
	      if (result != ASN1_SUCCESS)
		goto cleanup;

	      tlen = strlen (temp);
	      if (tlen > 0)
		_asn1_set_value (p, temp, tlen + 1);
	      counter += len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_GENERALIZED_TIME:
	    case ASN1_ETYPE_UTC_TIME:
	      result =
		_asn1_get_time_der (der + counter, len - counter, &len2, temp,
				    sizeof (temp) - 1);
	      if (result != ASN1_SUCCESS)
		goto cleanup;

	      tlen = strlen (temp);
	      if (tlen > 0)
		_asn1_set_value (p, temp, tlen);
	      counter += len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_OCTET_STRING:
	      len3 = len - counter;
	      result = _asn1_get_octet_string (der + counter, p, &len3);
	      if (result != ASN1_SUCCESS)
		goto cleanup;

	      counter += len3;
	      move = RIGHT;
	      break;
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
	      len2 =
		asn1_get_length_der (der + counter, len - counter, &len3);
	      if (len2 < 0)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}

	      _asn1_set_value (p, der + counter, len3 + len2);
	      counter += len3 + len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_SEQUENCE:
	    case ASN1_ETYPE_SET:
	      if (move == UP)
		{
		  len2 = _asn1_strtol (p->value, NULL, 10);
		  _asn1_set_value (p, NULL, 0);
		  if (len2 == -1)
		    {		/* indefinite length method */
		      if (len - counter + 1 > 0)
			{
			  if ((der[counter]) || der[counter + 1])
			    {
			      result = ASN1_DER_ERROR;
			      goto cleanup;
			    }
			}
		      else
			{
			  result = ASN1_DER_ERROR;
			  goto cleanup;
			}
		      counter += 2;
		    }
		  else
		    {		/* definite length method */
		      if (len2 != counter)
			{
			  result = ASN1_DER_ERROR;
			  goto cleanup;
			}
		    }
		  move = RIGHT;
		}
	      else
		{		/* move==DOWN || move==RIGHT */
		  len3 =
		    asn1_get_length_der (der + counter, len - counter, &len2);
		  if (len3 < -1)
		    {
		      result = ASN1_DER_ERROR;
		      goto cleanup;
		    }
		  counter += len2;
		  if (len3 > 0)
		    {
		      _asn1_ltostr (counter + len3, temp);
		      tlen = strlen (temp);
		      if (tlen > 0)
			_asn1_set_value (p, temp, tlen + 1);
		      move = DOWN;
		    }
		  else if (len3 == 0)
		    {
		      p2 = p->down;
		      while (p2)
			{
			  if (type_field (p2->type) != ASN1_ETYPE_TAG)
			    {
			      p3 = p2->right;
			      asn1_delete_structure (&p2);
			      p2 = p3;
			    }
			  else
			    p2 = p2->right;
			}
		      move = RIGHT;
		    }
		  else
		    {		/* indefinite length method */
		      _asn1_set_value (p, "-1", 3);
		      move = DOWN;
		    }
		}
	      break;
	    case ASN1_ETYPE_SEQUENCE_OF:
	    case ASN1_ETYPE_SET_OF:
	      if (move == UP)
		{
		  len2 = _asn1_strtol (p->value, NULL, 10);
		  if (len2 == -1)
		    {		/* indefinite length method */
		      if ((counter + 2) > len)
			{
			  result = ASN1_DER_ERROR;
			  goto cleanup;
			}

		      if ((der[counter]) || der[counter + 1])
			{
			  _asn1_append_sequence_set (p);
			  p = p->down;
			  while (p->right)
			    p = p->right;
			  move = RIGHT;
			  continue;
			}
		      _asn1_set_value (p, NULL, 0);
		      counter += 2;
		    }
		  else
		    {		/* definite length method */
		      if (len2 > counter)
			{
			  _asn1_append_sequence_set (p);
			  p = p->down;
			  while (p->right)
			    p = p->right;
			  move = RIGHT;
			  continue;
			}
		      _asn1_set_value (p, NULL, 0);
		      if (len2 != counter)
			{
			  result = ASN1_DER_ERROR;
			  goto cleanup;
			}
		    }
		}
	      else
		{		/* move==DOWN || move==RIGHT */
		  len3 =
		    asn1_get_length_der (der + counter, len - counter, &len2);
		  if (len3 < -1)
		    {
		      result = ASN1_DER_ERROR;
		      goto cleanup;
		    }
		  counter += len2;
		  if (len3)
		    {
		      if (len3 > 0)
			{	/* definite length method */
			  _asn1_ltostr (counter + len3, temp);
			  tlen = strlen (temp);

			  if (tlen > 0)
			    _asn1_set_value (p, temp, tlen + 1);
			}
		      else
			{	/* indefinite length method */
			  _asn1_set_value (p, "-1", 3);
			}
		      p2 = p->down;
		      while ((type_field (p2->type) == ASN1_ETYPE_TAG)
			     || (type_field (p2->type) == ASN1_ETYPE_SIZE))
			p2 = p2->right;
		      if (p2->right == NULL)
			_asn1_append_sequence_set (p);
		      p = p2;
		    }
		}
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_ANY:
	      if (asn1_get_tag_der
		  (der + counter, len - counter, &class, &len2,
		   &tag) != ASN1_SUCCESS)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}

	      if (counter + len2 > len)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}
	      len4 =
		asn1_get_length_der (der + counter + len2,
				     len - counter - len2, &len3);
	      if (len4 < -1)
		{
		  result = ASN1_DER_ERROR;
		  goto cleanup;
		}
	      if (len4 != -1)
		{
		  len2 += len4;
		  _asn1_set_value_lv (p, der + counter, len2 + len3);
		  counter += len2 + len3;
		}
	      else
		{		/* indefinite length */
		  /* Check indefinite lenth method in an EXPLICIT TAG */
		  if ((p->type & CONST_TAG) && (der[counter - 1] == 0x80))
		    indefinite = 1;
		  else
		    indefinite = 0;

		  len2 = len - counter;
		  result =
		    _asn1_get_indefinite_length_string (der + counter, &len2);
		  if (result != ASN1_SUCCESS)
		    goto cleanup;

		  _asn1_set_value_lv (p, der + counter, len2);
		  counter += len2;

		  /* Check if a couple of 0x00 are present due to an EXPLICIT TAG with
		     an indefinite length method. */
		  if (indefinite)
		    {
		      if (!der[counter] && !der[counter + 1])
			{
			  counter += 2;
			}
		      else
			{
			  result = ASN1_DER_ERROR;
			  goto cleanup;
			}
		    }
		}
	      move = RIGHT;
	      break;
	    default:
	      move = (move == UP) ? RIGHT : DOWN;
	      break;
	    }
	}

      if (p == node && move != DOWN)
	break;

      if (move == DOWN)
	{
	  if (p->down)
	    p = p->down;
	  else
	    move = RIGHT;
	}
      if ((move == RIGHT) && !(p->type & CONST_SET))
	{
	  if (p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }

  _asn1_delete_not_used (*element);

  if (counter != len)
    {
      result = ASN1_DER_ERROR;
      goto cleanup;
    }

  return ASN1_SUCCESS;

cleanup:
  asn1_delete_structure (element);
  return result;
}
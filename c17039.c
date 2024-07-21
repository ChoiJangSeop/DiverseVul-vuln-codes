StringEnd()
{
  struct canvas *cv;
  char *p;
  int typ;

  curr->w_state = LIT;
  *curr->w_stringp = '\0';
  switch (curr->w_StringType)
    {
    case OSC:	/* special xterm compatibility hack */
      if (curr->w_string[0] == ';' || (p = index(curr->w_string, ';')) == 0)
	break;
      typ = atoi(curr->w_string);
      p++;
#ifdef MULTIUSER
      if (typ == 83)	/* 83 = 'S' */
	{
	  /* special execute commands sequence */
	  char *args[MAXARGS];
	  int argl[MAXARGS];
	  struct acluser *windowuser;

	  windowuser = *FindUserPtr(":window:");
	  if (windowuser && Parse(p, sizeof(curr->w_string) - (p - curr->w_string), args, argl))
	    {
	      for (display = displays; display; display = display->d_next)
		if (D_forecv->c_layer->l_bottom == &curr->w_layer)
		  break;	/* found it */
	      if (display == 0 && curr->w_layer.l_cvlist)
		display = curr->w_layer.l_cvlist->c_display;
	      if (display == 0)
		display = displays;
	      EffectiveAclUser = windowuser;
	      fore = curr;
	      flayer = fore->w_savelayer ? fore->w_savelayer : &fore->w_layer;
	      DoCommand(args, argl);
	      EffectiveAclUser = 0;
	      fore = 0;
	      flayer = 0;
	    }
	  break;
	}
#endif
#ifdef RXVT_OSC
      if (typ == 0 || typ == 1 || typ == 2 || typ == 20 || typ == 39 || typ == 49)
	{
	  int typ2;
	  typ2 = typ / 10;
	  if (--typ2 < 0)
	    typ2 = 0;
	  if (strcmp(curr->w_xtermosc[typ2], p))
	    {
	      strncpy(curr->w_xtermosc[typ2], p, sizeof(curr->w_xtermosc[typ2]) - 1);
	      curr->w_xtermosc[typ2][sizeof(curr->w_xtermosc[typ2]) - 1] = 0;

	      for (display = displays; display; display = display->d_next)
		{
		  if (!D_CXT)
		    continue;
		  if (D_forecv->c_layer->l_bottom == &curr->w_layer)
		    SetXtermOSC(typ2, curr->w_xtermosc[typ2]);
		  if ((typ2 == 2 || typ2 == 3) && D_xtermosc[typ2])
		    Redisplay(0);
		}
	    }
	}
      if (typ != 0 && typ != 2)
	break;
#else
      if (typ < 0 || typ > 2)
	break;
#endif
      
      curr->w_stringp -= p - curr->w_string;
      if (curr->w_stringp > curr->w_string)
	bcopy(p, curr->w_string, curr->w_stringp - curr->w_string);
      *curr->w_stringp = '\0';
      /* FALLTHROUGH */
    case APC:
      if (curr->w_hstatus)
	{
	  if (strcmp(curr->w_hstatus, curr->w_string) == 0)
	    break;	/* not changed */
	  free(curr->w_hstatus);
	  curr->w_hstatus = 0;
	}
      if (curr->w_string != curr->w_stringp)
	curr->w_hstatus = SaveStr(curr->w_string);
      WindowChanged(curr, 'h');
      break;
    case PM:
    case GM:
      for (display = displays; display; display = display->d_next)
	{
	  for (cv = D_cvlist; cv; cv = cv->c_next)
	    if (cv->c_layer->l_bottom == &curr->w_layer)
	      break;
	  if (cv || curr->w_StringType == GM)
	    MakeStatus(curr->w_string);
	}
      return -1;
    case DCS:
      LAY_DISPLAYS(&curr->w_layer, AddStr(curr->w_string));
      break;
    case AKA:
      if (curr->w_title == curr->w_akabuf && !*curr->w_string)
	break;
      if (curr->w_dynamicaka)
	ChangeAKA(curr, curr->w_string, strlen(curr->w_string));
      if (!*curr->w_string)
	curr->w_autoaka = curr->w_y + 1;
      break;
    default:
      break;
    }
  return 0;
}
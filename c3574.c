readconf_print(uschar *name, uschar *type, BOOL no_labels)
{
BOOL names_only = FALSE;
optionlist *ol;
optionlist *ol2 = NULL;
driver_instance *d = NULL;
macro_item *m;
int size = 0;

if (type == NULL)
  {
  if (*name == '+')
    {
    int i;
    tree_node *t;
    BOOL found = FALSE;
    static uschar *types[] = { US"address", US"domain", US"host",
      US"localpart" };
    static tree_node **anchors[] = { &addresslist_anchor, &domainlist_anchor,
      &hostlist_anchor, &localpartlist_anchor };

    for (i = 0; i < 4; i++)
      {
      t = tree_search(*(anchors[i]), name+1);
      if (t != NULL)
        {
        found = TRUE;
        if (no_labels)
          printf("%s\n", ((namedlist_block *)(t->data.ptr))->string);
        else
          printf("%slist %s = %s\n", types[i], name+1,
            ((namedlist_block *)(t->data.ptr))->string);
        }
      }

    if (!found)
      printf("no address, domain, host, or local part list called \"%s\" "
        "exists\n", name+1);

    return;
    }

  if (Ustrcmp(name, "configure_file") == 0)
    {
    printf("%s\n", CS config_main_filename);
    return;
    }

  if (Ustrcmp(name, "all") == 0)
    {
    for (ol = optionlist_config;
         ol < optionlist_config + optionlist_config_size; ol++)
      {
      if ((ol->type & opt_hidden) == 0)
        print_ol(ol, US ol->name, NULL,
            optionlist_config, optionlist_config_size,
            no_labels);
      }
    return;
    }

  if (Ustrcmp(name, "local_scan") == 0)
    {
    #ifndef LOCAL_SCAN_HAS_OPTIONS
    printf("local_scan() options are not supported\n");
    #else
    for (ol = local_scan_options;
         ol < local_scan_options + local_scan_options_count; ol++)
      {
      print_ol(ol, US ol->name, NULL, local_scan_options,
        local_scan_options_count, no_labels);
      }
    #endif
    return;
    }

  if (Ustrcmp(name, "routers") == 0)
    {
    type = US"router";
    name = NULL;
    }
  else if (Ustrcmp(name, "transports") == 0)
    {
    type = US"transport";
    name = NULL;
    }

  else if (Ustrcmp(name, "authenticators") == 0)
    {
    type = US"authenticator";
    name = NULL;
    }

  else if (Ustrcmp(name, "macros") == 0)
    {
    type = US"macro";
    name = NULL;
    }

  else if (Ustrcmp(name, "router_list") == 0)
    {
    type = US"router";
    name = NULL;
    names_only = TRUE;
    }

  else if (Ustrcmp(name, "transport_list") == 0)
    {
    type = US"transport";
    name = NULL;
    names_only = TRUE;
    }

  else if (Ustrcmp(name, "authenticator_list") == 0)
    {
    type = US"authenticator";
    name = NULL;
    names_only = TRUE;
    }

  else if (Ustrcmp(name, "macro_list") == 0)
    {
    type = US"macro";
    name = NULL;
    names_only = TRUE;
    }

  else if (Ustrcmp(name, "environment") == 0)
    {
    if (environ)
      {
      uschar **p;
      size_t n;
      for (p = USS environ; *p; p++) ;
      n = p - USS environ;
      qsort(environ, p - USS environ, sizeof(*p), (__compar_fn_t) string_compare_by_pointer);

      for (p = USS environ; *p; p++)
        {
        if (no_labels) *(Ustrchr(*p, '=')) = '\0';
        puts(*p);
        }
      }
    return;
    }

  else
    {
    print_ol(find_option(name, optionlist_config, optionlist_config_size),
      name, NULL, optionlist_config, optionlist_config_size, no_labels);
    return;
    }
  }

/* Handle the options for a router or transport. Skip options that are flagged
as hidden. Some of these are options with names starting with '*', used for
internal alternative representations of other options (which the printing
function will sort out). Others are synonyms kept for backward compatibility.
*/

if (Ustrcmp(type, "router") == 0)
  {
  d = (driver_instance *)routers;
  ol2 = optionlist_routers;
  size = optionlist_routers_size;
  }
else if (Ustrcmp(type, "transport") == 0)
  {
  d = (driver_instance *)transports;
  ol2 = optionlist_transports;
  size = optionlist_transports_size;
  }
else if (Ustrcmp(type, "authenticator") == 0)
  {
  d = (driver_instance *)auths;
  ol2 = optionlist_auths;
  size = optionlist_auths_size;
  }

else if (Ustrcmp(type, "macro") == 0)
  {
  /* People store passwords in macros and they were previously not available
  for printing.  So we have an admin_users restriction. */
  if (!admin_user)
    {
    fprintf(stderr, "exim: permission denied\n");
    exit(EXIT_FAILURE);
    }
  for (m = macros; m != NULL; m = m->next)
    {
    if (name == NULL || Ustrcmp(name, m->name) == 0)
      {
      if (names_only)
        printf("%s\n", CS m->name);
      else
        printf("%s=%s\n", CS m->name, CS m->replacement);
      if (name != NULL)
        return;
      }
    }
  if (name != NULL)
    printf("%s %s not found\n", type, name);
  return;
  }

if (names_only)
  {
  for (; d != NULL; d = d->next) printf("%s\n", CS d->name);
  return;
  }

/* Either search for a given driver, or print all of them */

for (; d != NULL; d = d->next)
  {
  if (name == NULL)
    printf("\n%s %s:\n", d->name, type);
  else if (Ustrcmp(d->name, name) != 0) continue;

  for (ol = ol2; ol < ol2 + size; ol++)
    {
    if ((ol->type & opt_hidden) == 0)
      print_ol(ol, US ol->name, d, ol2, size, no_labels);
    }

  for (ol = d->info->options;
       ol < d->info->options + *(d->info->options_count); ol++)
    {
    if ((ol->type & opt_hidden) == 0)
      print_ol(ol, US ol->name, d, d->info->options, *(d->info->options_count), no_labels);
    }
  if (name != NULL) return;
  }
if (name != NULL) printf("%s %s not found\n", type, name);
}
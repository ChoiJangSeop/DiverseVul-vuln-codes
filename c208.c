_dbus_marshal_validate_test (void)
{
  DBusString str;
  int i;

  const char *valid_paths[] = {
    "/",
    "/foo/bar",
    "/foo",
    "/foo/bar/baz"
  };
  const char *invalid_paths[] = {
    "bar",
    "bar/baz",
    "/foo/bar/",
    "/foo/"
    "foo/",
    "boo//blah",
    "//",
    "///",
    "foo///blah/",
    "Hello World",
    "",
    "   ",
    "foo bar"
  };

  const char *valid_interfaces[] = {
    "org.freedesktop.Foo",
    "Bar.Baz",
    "Blah.Blah.Blah.Blah.Blah",
    "a.b",
    "a.b.c.d.e.f.g",
    "a0.b1.c2.d3.e4.f5.g6",
    "abc123.foo27"
  };
  const char *invalid_interfaces[] = {
    ".",
    "",
    "..",
    ".Foo.Bar",
    "..Foo.Bar",
    "Foo.Bar.",
    "Foo.Bar..",
    "Foo",
    "9foo.bar.baz",
    "foo.bar..baz",
    "foo.bar...baz",
    "foo.bar.b..blah",
    ":",
    ":0-1",
    "10",
    ":11.34324",
    "0.0.0",
    "0..0",
    "foo.Bar.%",
    "foo.Bar!!",
    "!Foo.bar.bz",
    "foo.$.blah",
    "",
    "   ",
    "foo bar"
  };

  const char *valid_unique_names[] = {
    ":0",
    ":a",
    ":",
    ":.a",
    ":.1",
    ":0.1",
    ":000.2222",
    ":.blah",
    ":abce.freedesktop.blah"
  };
  const char *invalid_unique_names[] = {
    //":-",
    ":!",
    //":0-10",
    ":blah.",
    ":blah.",
    ":blah..org",
    ":blah.org..",
    ":..blah.org",
    "",
    "   ",
    "foo bar"
  };

  const char *valid_members[] = {
    "Hello",
    "Bar",
    "foobar",
    "_foobar",
    "foo89"
  };

  const char *invalid_members[] = {
    "9Hello",
    "10",
    "1",
    "foo-bar",
    "blah.org",
    ".blah",
    "blah.",
    "Hello.",
    "!foo",
    "",
    "   ",
    "foo bar"
  };

  const char *valid_signatures[] = {
    "",
    "sss",
    "i",
    "b"
  };

  const char *invalid_signatures[] = {
    " ",
    "not a valid signature",
    "123",
    ".",
    "("
  };

  /* Signature with reason */

  run_validity_tests (signature_tests, _DBUS_N_ELEMENTS (signature_tests),
                      _dbus_validate_signature_with_reason);

  /* Path validation */
  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (valid_paths))
    {
      _dbus_string_init_const (&str, valid_paths[i]);

      if (!_dbus_validate_path (&str, 0,
                                _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Path \"%s\" should have been valid\n", valid_paths[i]);
          _dbus_assert_not_reached ("invalid path");
        }

      ++i;
    }

  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (invalid_paths))
    {
      _dbus_string_init_const (&str, invalid_paths[i]);

      if (_dbus_validate_path (&str, 0,
                               _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Path \"%s\" should have been invalid\n", invalid_paths[i]);
          _dbus_assert_not_reached ("valid path");
        }

      ++i;
    }

  /* Interface validation */
  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (valid_interfaces))
    {
      _dbus_string_init_const (&str, valid_interfaces[i]);

      if (!_dbus_validate_interface (&str, 0,
                                     _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Interface \"%s\" should have been valid\n", valid_interfaces[i]);
          _dbus_assert_not_reached ("invalid interface");
        }

      ++i;
    }

  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (invalid_interfaces))
    {
      _dbus_string_init_const (&str, invalid_interfaces[i]);

      if (_dbus_validate_interface (&str, 0,
                                    _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Interface \"%s\" should have been invalid\n", invalid_interfaces[i]);
          _dbus_assert_not_reached ("valid interface");
        }

      ++i;
    }

  /* Bus name validation (check that valid interfaces are valid bus names,
   * and invalid interfaces are invalid services except if they start with ':')
   */
  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (valid_interfaces))
    {
      _dbus_string_init_const (&str, valid_interfaces[i]);

      if (!_dbus_validate_bus_name (&str, 0,
                                   _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Bus name \"%s\" should have been valid\n", valid_interfaces[i]);
          _dbus_assert_not_reached ("invalid bus name");
        }

      ++i;
    }

  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (invalid_interfaces))
    {
      if (invalid_interfaces[i][0] != ':')
        {
          _dbus_string_init_const (&str, invalid_interfaces[i]);

          if (_dbus_validate_bus_name (&str, 0,
                                       _dbus_string_get_length (&str)))
            {
              _dbus_warn ("Bus name \"%s\" should have been invalid\n", invalid_interfaces[i]);
              _dbus_assert_not_reached ("valid bus name");
            }
        }

      ++i;
    }

  /* unique name validation */
  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (valid_unique_names))
    {
      _dbus_string_init_const (&str, valid_unique_names[i]);

      if (!_dbus_validate_bus_name (&str, 0,
                                    _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Bus name \"%s\" should have been valid\n", valid_unique_names[i]);
          _dbus_assert_not_reached ("invalid unique name");
        }

      ++i;
    }

  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (invalid_unique_names))
    {
      _dbus_string_init_const (&str, invalid_unique_names[i]);

      if (_dbus_validate_bus_name (&str, 0,
                                   _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Bus name \"%s\" should have been invalid\n", invalid_unique_names[i]);
          _dbus_assert_not_reached ("valid unique name");
        }

      ++i;
    }


  /* Error name validation (currently identical to interfaces)
   */
  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (valid_interfaces))
    {
      _dbus_string_init_const (&str, valid_interfaces[i]);

      if (!_dbus_validate_error_name (&str, 0,
                                      _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Error name \"%s\" should have been valid\n", valid_interfaces[i]);
          _dbus_assert_not_reached ("invalid error name");
        }

      ++i;
    }

  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (invalid_interfaces))
    {
      if (invalid_interfaces[i][0] != ':')
        {
          _dbus_string_init_const (&str, invalid_interfaces[i]);

          if (_dbus_validate_error_name (&str, 0,
                                         _dbus_string_get_length (&str)))
            {
              _dbus_warn ("Error name \"%s\" should have been invalid\n", invalid_interfaces[i]);
              _dbus_assert_not_reached ("valid error name");
            }
        }

      ++i;
    }

  /* Member validation */
  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (valid_members))
    {
      _dbus_string_init_const (&str, valid_members[i]);

      if (!_dbus_validate_member (&str, 0,
                                  _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Member \"%s\" should have been valid\n", valid_members[i]);
          _dbus_assert_not_reached ("invalid member");
        }

      ++i;
    }

  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (invalid_members))
    {
      _dbus_string_init_const (&str, invalid_members[i]);

      if (_dbus_validate_member (&str, 0,
                                 _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Member \"%s\" should have been invalid\n", invalid_members[i]);
          _dbus_assert_not_reached ("valid member");
        }

      ++i;
    }

  /* Signature validation */
  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (valid_signatures))
    {
      _dbus_string_init_const (&str, valid_signatures[i]);

      if (!_dbus_validate_signature (&str, 0,
                                     _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Signature \"%s\" should have been valid\n", valid_signatures[i]);
          _dbus_assert_not_reached ("invalid signature");
        }

      ++i;
    }

  i = 0;
  while (i < (int) _DBUS_N_ELEMENTS (invalid_signatures))
    {
      _dbus_string_init_const (&str, invalid_signatures[i]);

      if (_dbus_validate_signature (&str, 0,
                                    _dbus_string_get_length (&str)))
        {
          _dbus_warn ("Signature \"%s\" should have been invalid\n", invalid_signatures[i]);
          _dbus_assert_not_reached ("valid signature");
        }

      ++i;
    }

  /* Validate claimed length longer than real length */
  _dbus_string_init_const (&str, "abc.efg");
  if (_dbus_validate_bus_name (&str, 0, 8))
    _dbus_assert_not_reached ("validated too-long string");
  if (_dbus_validate_interface (&str, 0, 8))
    _dbus_assert_not_reached ("validated too-long string");
  if (_dbus_validate_error_name (&str, 0, 8))
    _dbus_assert_not_reached ("validated too-long string");

  _dbus_string_init_const (&str, "abc");
  if (_dbus_validate_member (&str, 0, 4))
    _dbus_assert_not_reached ("validated too-long string");

  _dbus_string_init_const (&str, "sss");
  if (_dbus_validate_signature (&str, 0, 4))
    _dbus_assert_not_reached ("validated too-long signature");

  /* Validate string exceeding max name length */
  if (!_dbus_string_init (&str))
    _dbus_assert_not_reached ("no memory");

  while (_dbus_string_get_length (&str) <= DBUS_MAXIMUM_NAME_LENGTH)
    if (!_dbus_string_append (&str, "abc.def"))
      _dbus_assert_not_reached ("no memory");

  if (_dbus_validate_bus_name (&str, 0, _dbus_string_get_length (&str)))
    _dbus_assert_not_reached ("validated overmax string");
  if (_dbus_validate_interface (&str, 0, _dbus_string_get_length (&str)))
    _dbus_assert_not_reached ("validated overmax string");
  if (_dbus_validate_error_name (&str, 0, _dbus_string_get_length (&str)))
    _dbus_assert_not_reached ("validated overmax string");

  /* overlong member */
  _dbus_string_set_length (&str, 0);
  while (_dbus_string_get_length (&str) <= DBUS_MAXIMUM_NAME_LENGTH)
    if (!_dbus_string_append (&str, "abc"))
      _dbus_assert_not_reached ("no memory");

  if (_dbus_validate_member (&str, 0, _dbus_string_get_length (&str)))
    _dbus_assert_not_reached ("validated overmax string");

  /* overlong unique name */
  _dbus_string_set_length (&str, 0);
  _dbus_string_append (&str, ":");
  while (_dbus_string_get_length (&str) <= DBUS_MAXIMUM_NAME_LENGTH)
    if (!_dbus_string_append (&str, "abc"))
      _dbus_assert_not_reached ("no memory");

  if (_dbus_validate_bus_name (&str, 0, _dbus_string_get_length (&str)))
    _dbus_assert_not_reached ("validated overmax string");

  _dbus_string_free (&str);

  /* Body validation; test basic validation of valid bodies for both endian */
  
  {
    int sequence;
    DBusString signature;
    DBusString body;

    if (!_dbus_string_init (&signature) || !_dbus_string_init (&body))
      _dbus_assert_not_reached ("oom");

    sequence = 0;
    while (dbus_internal_do_not_use_generate_bodies (sequence,
                                                     DBUS_LITTLE_ENDIAN,
                                                     &signature, &body))
      {
        DBusValidity validity;

        validity = _dbus_validate_body_with_reason (&signature, 0,
                                                    DBUS_LITTLE_ENDIAN,
                                                    NULL, &body, 0,
                                                    _dbus_string_get_length (&body));
        if (validity != DBUS_VALID)
          {
            _dbus_warn ("invalid code %d expected valid on sequence %d little endian\n",
                        validity, sequence);
            _dbus_verbose_bytes_of_string (&signature, 0, _dbus_string_get_length (&signature));
            _dbus_verbose_bytes_of_string (&body, 0, _dbus_string_get_length (&body));
            _dbus_assert_not_reached ("test failed");
          }

        _dbus_string_set_length (&signature, 0);
        _dbus_string_set_length (&body, 0);
        ++sequence;
      }
                                                     
    sequence = 0;
    while (dbus_internal_do_not_use_generate_bodies (sequence,
                                                     DBUS_BIG_ENDIAN,
                                                     &signature, &body))
      {
        DBusValidity validity;

        validity = _dbus_validate_body_with_reason (&signature, 0,
                                                    DBUS_BIG_ENDIAN,
                                                    NULL, &body, 0,
                                                    _dbus_string_get_length (&body));
        if (validity != DBUS_VALID)
          {
            _dbus_warn ("invalid code %d expected valid on sequence %d big endian\n",
                        validity, sequence);
            _dbus_verbose_bytes_of_string (&signature, 0, _dbus_string_get_length (&signature));
            _dbus_verbose_bytes_of_string (&body, 0, _dbus_string_get_length (&body));
            _dbus_assert_not_reached ("test failed");
          }

        _dbus_string_set_length (&signature, 0);
        _dbus_string_set_length (&body, 0);
        ++sequence;
      }

    _dbus_string_free (&signature);
    _dbus_string_free (&body);
  }
  
  return TRUE;
}
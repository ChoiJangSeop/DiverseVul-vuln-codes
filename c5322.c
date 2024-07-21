static void test_modules()
{
  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.constants.one + 1 == tests.constants.two \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.constants.foo == \"foo\" \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.constants.empty == \"\"  \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.empty() == \"\"  \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.struct_array[1].i == 1  \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.struct_array[0].i == 1 or true \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.integer_array[0] == 0 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.integer_array[1] == 1 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.string_array[0] == \"foo\" \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.string_array[2] == \"baz\" \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.string_dict[\"foo\"] == \"foo\" \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.string_dict[\"bar\"] == \"bar\" \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.isum(1,2) == 3 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.isum(1,2,3) == 6 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.fsum(1.0,2.0) == 3.0 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.fsum(1.0,2.0,3.0) == 6.0 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.foobar(1) == tests.foobar(1) \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.foobar(1) != tests.foobar(2) \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
       rule test { \
        condition: tests.length(\"dummy\") == 5 \
      }",
      NULL);


  assert_false_rule(
      "import \"tests\" \
      rule test { condition: tests.struct_array[0].i == 1  \
      }",
      NULL);

  assert_false_rule(
      "import \"tests\" \
      rule test { condition: tests.isum(1,1) == 3 \
      }",
      NULL);

  assert_false_rule(
      "import \"tests\" \
      rule test { condition: tests.fsum(1.0,1.0) == 3.0 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
      rule test { condition: tests.match(/foo/,\"foo\") == 3 \
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
      rule test { condition: tests.match(/foo/,\"bar\") == -1\
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
      rule test { condition: tests.match(/foo.bar/i,\"FOO\\nBAR\") == -1\
      }",
      NULL);

  assert_true_rule(
      "import \"tests\" \
      rule test { condition: tests.match(/foo.bar/is,\"FOO\\nBAR\") == 7\
      }",
      NULL);

  assert_error(
      "import \"\\x00\"",
      ERROR_INVALID_MODULE_NAME);

  assert_error(
      "import \"\"",
      ERROR_INVALID_MODULE_NAME);
}
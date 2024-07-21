test_bson_append_code_with_scope (void)
{
   const uint8_t *scope_buf = NULL;
   uint32_t scopelen = 0;
   uint32_t len = 0;
   bson_iter_t iter;
   bool r;
   const char *code = NULL;
   bson_t *b;
   bson_t *b2;
   bson_t *scope;

   /* Test with NULL bson, which converts to just CODE type. */
   b = bson_new ();
   BSON_ASSERT (
      bson_append_code_with_scope (b, "code", -1, "var a = {};", NULL));
   b2 = get_bson ("test30.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   r = bson_iter_init_find (&iter, b, "code");
   BSON_ASSERT (r);
   BSON_ASSERT (BSON_ITER_HOLDS_CODE (&iter)); /* Not codewscope */
   bson_destroy (b);
   bson_destroy (b2);

   /* Empty scope is still CODEWSCOPE. */
   b = bson_new ();
   scope = bson_new ();
   BSON_ASSERT (
      bson_append_code_with_scope (b, "code", -1, "var a = {};", scope));
   b2 = get_bson ("code_w_empty_scope.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   r = bson_iter_init_find (&iter, b, "code");
   BSON_ASSERT (r);
   BSON_ASSERT (BSON_ITER_HOLDS_CODEWSCOPE (&iter));
   bson_destroy (b);
   bson_destroy (b2);
   bson_destroy (scope);

   /* Test with non-empty scope */
   b = bson_new ();
   scope = bson_new ();
   BSON_ASSERT (bson_append_utf8 (scope, "foo", -1, "bar", -1));
   BSON_ASSERT (
      bson_append_code_with_scope (b, "code", -1, "var a = {};", scope));
   b2 = get_bson ("test31.bson");
   BSON_ASSERT_BSON_EQUAL (b, b2);
   r = bson_iter_init_find (&iter, b, "code");
   BSON_ASSERT (r);
   BSON_ASSERT (BSON_ITER_HOLDS_CODEWSCOPE (&iter));
   code = bson_iter_codewscope (&iter, &len, &scopelen, &scope_buf);
   BSON_ASSERT (len == 11);
   BSON_ASSERT (scopelen == scope->len);
   BSON_ASSERT (!strcmp (code, "var a = {};"));
   bson_destroy (b);
   bson_destroy (b2);
   bson_destroy (scope);
}
static void test_jwt_kid_escape(void)
{
	test_begin("JWT kid escape");
	/* save a token */
	 buffer_t *secret = t_buffer_create(32);
	 void *ptr = buffer_append_space_unsafe(secret, 32);
	 random_fill(ptr, 32);
	 buffer_t *b64_key = t_base64_encode(0, SIZE_MAX,
					     secret->data, secret->used);
	 save_key_to("HS256", "hello%2eworld%2f%25", str_c(b64_key));
	/* make a token */
	buffer_t *tokenbuf = create_jwt_token_kid("HS256", "hello.world/%");
	/* sign it */
	sign_jwt_token_hs256(tokenbuf, secret);
	test_jwt_token(str_c(tokenbuf));
	test_end();
}
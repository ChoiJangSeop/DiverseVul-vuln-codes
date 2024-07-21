static int do_drbg_all(void)
    {
    static DRBG_LIST drbg_types[] =
	{
		{NID_sha1, 0},
		{NID_sha224, 0},
		{NID_sha256, 0},
		{NID_sha384, 0},
		{NID_sha512, 0},
		{NID_hmacWithSHA1, 0},
		{NID_hmacWithSHA224, 0},
		{NID_hmacWithSHA256, 0},
		{NID_hmacWithSHA384, 0},
		{NID_hmacWithSHA512, 0},
		{NID_aes_128_ctr, 0},
		{NID_aes_192_ctr, 0},
		{NID_aes_256_ctr, 0},
		{NID_aes_128_ctr, DRBG_FLAG_CTR_USE_DF},
		{NID_aes_192_ctr, DRBG_FLAG_CTR_USE_DF},
		{NID_aes_256_ctr, DRBG_FLAG_CTR_USE_DF},
		{(NID_X9_62_prime256v1 << 16)|NID_sha1, 0},
		{(NID_X9_62_prime256v1 << 16)|NID_sha224, 0},
		{(NID_X9_62_prime256v1 << 16)|NID_sha256, 0},
		{(NID_X9_62_prime256v1 << 16)|NID_sha384, 0},
		{(NID_X9_62_prime256v1 << 16)|NID_sha512, 0},
		{(NID_secp384r1 << 16)|NID_sha224, 0},
		{(NID_secp384r1 << 16)|NID_sha256, 0},
		{(NID_secp384r1 << 16)|NID_sha384, 0},
		{(NID_secp384r1 << 16)|NID_sha512, 0},
		{(NID_secp521r1 << 16)|NID_sha256, 0},
		{(NID_secp521r1 << 16)|NID_sha384, 0},
		{(NID_secp521r1 << 16)|NID_sha512, 0},
		{0, 0}
	};
    DRBG_LIST *lst;
    int rv = 1;
    for (lst = drbg_types;; lst++)
	{
	if (lst->type == 0)
		break;
	if (!do_drbg_test(lst->type, lst->flags))
		rv = 0;
	}
    return rv;
    }
char ssl3_cbc_record_digest_supported(const EVP_MD *digest)
	{
	switch (digest->type)
		{
		case NID_md5:
		case NID_sha1:
		case NID_sha224:
		case NID_sha256:
		case NID_sha384:
		case NID_sha512:
			return 1;
		default:
			return 0;
		}
	}
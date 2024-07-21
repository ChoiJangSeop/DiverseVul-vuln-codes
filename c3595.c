int main(int argc, char **argv)
{
	int len;
	uint8_t *encoded;

#define DNAME_DEC(encoded,pre) dname_dec((uint8_t*)(encoded), sizeof(encoded), (pre))
	printf("'%s'\n",       DNAME_DEC("\4host\3com\0", "test1:"));
	printf("test2:'%s'\n", DNAME_DEC("\4host\3com\0\4host\3com\0", ""));
	printf("test3:'%s'\n", DNAME_DEC("\4host\3com\0\xC0\0", ""));
	printf("test4:'%s'\n", DNAME_DEC("\4host\3com\0\xC0\5", ""));
	printf("test5:'%s'\n", DNAME_DEC("\4host\3com\0\xC0\5\1z\xC0\xA", ""));

#define DNAME_ENC(cache,source,lenp) dname_enc((uint8_t*)(cache), sizeof(cache), (source), (lenp))
	encoded = dname_enc(NULL, 0, "test.net", &len);
	printf("test6:'%s' len:%d\n", dname_dec(encoded, len, ""), len);
	encoded = DNAME_ENC("\3net\0", "test.net", &len);
	printf("test7:'%s' len:%d\n", dname_dec(encoded, len, ""), len);
	encoded = DNAME_ENC("\4test\3net\0", "test.net", &len);
	printf("test8:'%s' len:%d\n", dname_dec(encoded, len, ""), len);
	return 0;
}
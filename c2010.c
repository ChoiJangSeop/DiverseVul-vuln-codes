START_TEST(test_find_user_by_ip)
{
	in_addr_t ip;
	unsigned int testip;

	ip = inet_addr("127.0.0.1");
	init_users(ip, 27);
	users[0].conn = CONN_DNS_NULL;

	testip = (unsigned int) inet_addr("10.0.0.1");
	fail_unless(find_user_by_ip(testip) == -1);

	testip = (unsigned int) inet_addr("127.0.0.2");
	fail_unless(find_user_by_ip(testip) == -1);

	users[0].active = 1;

	testip = (unsigned int) inet_addr("127.0.0.2");
	fail_unless(find_user_by_ip(testip) == -1);

	users[0].last_pkt = time(NULL);

	testip = (unsigned int) inet_addr("127.0.0.2");
	fail_unless(find_user_by_ip(testip) == 0);
}
START_TEST(test_find_available_user)
{
	in_addr_t ip;
	int i;

	ip = inet_addr("127.0.0.1");
	init_users(ip, 27);

	for (i = 0; i < USERS; i++) {
		fail_unless(find_available_user() == i);
	}

	for (i = 0; i < USERS; i++) {
		fail_unless(find_available_user() == -1);
	}

	users[3].active = 0;

	fail_unless(find_available_user() == 3);
	fail_unless(find_available_user() == -1);

	users[3].last_pkt = 55;

	fail_unless(find_available_user() == 3);
	fail_unless(find_available_user() == -1);
}
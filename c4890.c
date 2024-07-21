test_policies_general(void *arg)
{
  int i;
  smartlist_t *policy = NULL, *policy2 = NULL, *policy3 = NULL,
              *policy4 = NULL, *policy5 = NULL, *policy6 = NULL,
              *policy7 = NULL, *policy8 = NULL, *policy9 = NULL,
              *policy10 = NULL, *policy11 = NULL, *policy12 = NULL;
  addr_policy_t *p;
  tor_addr_t tar, tar2;
  smartlist_t *addr_list = NULL;
  config_line_t line;
  smartlist_t *sm = NULL;
  char *policy_str = NULL;
  short_policy_t *short_parsed = NULL;
  int malformed_list = -1;
  (void)arg;

  policy = smartlist_new();

  p = router_parse_addr_policy_item_from_string("reject 192.168.0.0/16:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  tt_int_op(ADDR_POLICY_REJECT,OP_EQ, p->policy_type);
  tor_addr_from_ipv4h(&tar, 0xc0a80000u);
  tt_int_op(0,OP_EQ, tor_addr_compare(&p->addr, &tar, CMP_EXACT));
  tt_int_op(16,OP_EQ, p->maskbits);
  tt_int_op(1,OP_EQ, p->prt_min);
  tt_int_op(65535,OP_EQ, p->prt_max);

  smartlist_add(policy, p);

  tor_addr_from_ipv4h(&tar, 0x01020304u);
  tt_assert(ADDR_POLICY_ACCEPTED ==
          compare_tor_addr_to_addr_policy(&tar, 2, policy));
  tor_addr_make_unspec(&tar);
  tt_assert(ADDR_POLICY_PROBABLY_ACCEPTED ==
          compare_tor_addr_to_addr_policy(&tar, 2, policy));
  tor_addr_from_ipv4h(&tar, 0xc0a80102);
  tt_assert(ADDR_POLICY_REJECTED ==
          compare_tor_addr_to_addr_policy(&tar, 2, policy));

  tt_int_op(0, OP_EQ, policies_parse_exit_policy(NULL, &policy2,
                                              EXIT_POLICY_IPV6_ENABLED |
                                              EXIT_POLICY_REJECT_PRIVATE |
                                              EXIT_POLICY_ADD_DEFAULT, NULL));

  tt_assert(policy2);

  tor_addr_from_ipv4h(&tar, 0x0306090cu);
  tor_addr_parse(&tar2, "[2000::1234]");
  addr_list = smartlist_new();
  smartlist_add(addr_list, &tar);
  smartlist_add(addr_list, &tar2);
  tt_int_op(0, OP_EQ, policies_parse_exit_policy(NULL, &policy12,
                                                 EXIT_POLICY_IPV6_ENABLED |
                                                 EXIT_POLICY_REJECT_PRIVATE |
                                                 EXIT_POLICY_ADD_DEFAULT,
                                                 addr_list));
  smartlist_free(addr_list);
  addr_list = NULL;

  tt_assert(policy12);

  policy3 = smartlist_new();
  p = router_parse_addr_policy_item_from_string("reject *:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy3, p);
  p = router_parse_addr_policy_item_from_string("accept *:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy3, p);

  policy4 = smartlist_new();
  p = router_parse_addr_policy_item_from_string("accept *:443", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy4, p);
  p = router_parse_addr_policy_item_from_string("accept *:443", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy4, p);

  policy5 = smartlist_new();
  p = router_parse_addr_policy_item_from_string("reject 0.0.0.0/8:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject 169.254.0.0/16:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject 127.0.0.0/8:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject 192.168.0.0/16:*",
                                                -1, &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject 10.0.0.0/8:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject 172.16.0.0/12:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject 80.190.250.90:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject *:1-65534", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("reject *:65535", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);
  p = router_parse_addr_policy_item_from_string("accept *:1-65535", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy5, p);

  policy6 = smartlist_new();
  p = router_parse_addr_policy_item_from_string("accept 43.3.0.0/9:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy6, p);

  policy7 = smartlist_new();
  p = router_parse_addr_policy_item_from_string("accept 0.0.0.0/8:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy7, p);

  tt_int_op(0, OP_EQ, policies_parse_exit_policy(NULL, &policy8,
                                                 EXIT_POLICY_IPV6_ENABLED |
                                                 EXIT_POLICY_REJECT_PRIVATE |
                                                 EXIT_POLICY_ADD_DEFAULT,
                                                 NULL));

  tt_assert(policy8);

  tt_int_op(0, OP_EQ, policies_parse_exit_policy(NULL, &policy9,
                                                 EXIT_POLICY_REJECT_PRIVATE |
                                                 EXIT_POLICY_ADD_DEFAULT,
                                                 NULL));

  tt_assert(policy9);

  /* accept6 * and reject6 * produce IPv6 wildcards only */
  policy10 = smartlist_new();
  p = router_parse_addr_policy_item_from_string("accept6 *:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy10, p);

  policy11 = smartlist_new();
  p = router_parse_addr_policy_item_from_string("reject6 *:*", -1,
                                                &malformed_list);
  tt_assert(p != NULL);
  smartlist_add(policy11, p);

  tt_assert(!exit_policy_is_general_exit(policy));
  tt_assert(exit_policy_is_general_exit(policy2));
  tt_assert(!exit_policy_is_general_exit(NULL));
  tt_assert(!exit_policy_is_general_exit(policy3));
  tt_assert(!exit_policy_is_general_exit(policy4));
  tt_assert(!exit_policy_is_general_exit(policy5));
  tt_assert(!exit_policy_is_general_exit(policy6));
  tt_assert(!exit_policy_is_general_exit(policy7));
  tt_assert(exit_policy_is_general_exit(policy8));
  tt_assert(exit_policy_is_general_exit(policy9));
  tt_assert(!exit_policy_is_general_exit(policy10));
  tt_assert(!exit_policy_is_general_exit(policy11));

  tt_assert(cmp_addr_policies(policy, policy2));
  tt_assert(cmp_addr_policies(policy, NULL));
  tt_assert(!cmp_addr_policies(policy2, policy2));
  tt_assert(!cmp_addr_policies(NULL, NULL));

  tt_assert(!policy_is_reject_star(policy2, AF_INET, 1));
  tt_assert(policy_is_reject_star(policy, AF_INET, 1));
  tt_assert(policy_is_reject_star(policy10, AF_INET, 1));
  tt_assert(!policy_is_reject_star(policy10, AF_INET6, 1));
  tt_assert(policy_is_reject_star(policy11, AF_INET, 1));
  tt_assert(policy_is_reject_star(policy11, AF_INET6, 1));
  tt_assert(policy_is_reject_star(NULL, AF_INET, 1));
  tt_assert(policy_is_reject_star(NULL, AF_INET6, 1));
  tt_assert(!policy_is_reject_star(NULL, AF_INET, 0));
  tt_assert(!policy_is_reject_star(NULL, AF_INET6, 0));

  addr_policy_list_free(policy);
  policy = NULL;

  /* make sure assume_action works */
  malformed_list = 0;
  p = router_parse_addr_policy_item_from_string("127.0.0.1",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("127.0.0.1:*",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("[::]",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("[::]:*",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("[face::b]",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("[b::aaaa]",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("*",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("*4",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("*6",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(p);
  addr_policy_free(p);
  tt_assert(!malformed_list);

  /* These are all ambiguous IPv6 addresses, it's good that we reject them */
  p = router_parse_addr_policy_item_from_string("acce::abcd",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(!p);
  tt_assert(malformed_list);
  malformed_list = 0;

  p = router_parse_addr_policy_item_from_string("7:1234",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(!p);
  tt_assert(malformed_list);
  malformed_list = 0;

  p = router_parse_addr_policy_item_from_string("::",
                                                ADDR_POLICY_ACCEPT,
                                                &malformed_list);
  tt_assert(!p);
  tt_assert(malformed_list);
  malformed_list = 0;

  /* make sure compacting logic works. */
  policy = NULL;
  line.key = (char*)"foo";
  line.value = (char*)"accept *:80,reject private:*,reject *:*";
  line.next = NULL;
  tt_int_op(0, OP_EQ, policies_parse_exit_policy(&line,&policy,
                                              EXIT_POLICY_IPV6_ENABLED |
                                              EXIT_POLICY_ADD_DEFAULT, NULL));
  tt_assert(policy);

  //test_streq(policy->string, "accept *:80");
  //test_streq(policy->next->string, "reject *:*");
  tt_int_op(smartlist_len(policy),OP_EQ, 4);

  /* test policy summaries */
  /* check if we properly ignore private IP addresses */
  test_policy_summary_helper("reject 192.168.0.0/16:*,"
                             "reject 0.0.0.0/8:*,"
                             "reject 10.0.0.0/8:*,"
                             "accept *:10-30,"
                             "accept *:90,"
                             "reject *:*",
                             "accept 10-30,90");
  /* check all accept policies, and proper counting of rejects */
  test_policy_summary_helper("reject 11.0.0.0/9:80,"
                             "reject 12.0.0.0/9:80,"
                             "reject 13.0.0.0/9:80,"
                             "reject 14.0.0.0/9:80,"
                             "accept *:*", "accept 1-65535");
  test_policy_summary_helper("reject 11.0.0.0/9:80,"
                             "reject 12.0.0.0/9:80,"
                             "reject 13.0.0.0/9:80,"
                             "reject 14.0.0.0/9:80,"
                             "reject 15.0.0.0:81,"
                             "accept *:*", "accept 1-65535");
  test_policy_summary_helper6("reject 11.0.0.0/9:80,"
                              "reject 12.0.0.0/9:80,"
                              "reject 13.0.0.0/9:80,"
                              "reject 14.0.0.0/9:80,"
                              "reject 15.0.0.0:80,"
                              "accept *:*",
                              "reject 80",
                              "accept 1-65535");
  /* no exits */
  test_policy_summary_helper("accept 11.0.0.0/9:80,"
                             "reject *:*",
                             "reject 1-65535");
  /* port merging */
  test_policy_summary_helper("accept *:80,"
                             "accept *:81,"
                             "accept *:100-110,"
                             "accept *:111,"
                             "reject *:*",
                             "accept 80-81,100-111");
  /* border ports */
  test_policy_summary_helper("accept *:1,"
                             "accept *:3,"
                             "accept *:65535,"
                             "reject *:*",
                             "accept 1,3,65535");
  /* holes */
  test_policy_summary_helper("accept *:1,"
                             "accept *:3,"
                             "accept *:5,"
                             "accept *:7,"
                             "reject *:*",
                             "accept 1,3,5,7");
  test_policy_summary_helper("reject *:1,"
                             "reject *:3,"
                             "reject *:5,"
                             "reject *:7,"
                             "accept *:*",
                             "reject 1,3,5,7");
  /* long policies */
  /* standard long policy on many exits */
  test_policy_summary_helper("accept *:20-23,"
                             "accept *:43,"
                             "accept *:53,"
                             "accept *:79-81,"
                             "accept *:88,"
                             "accept *:110,"
                             "accept *:143,"
                             "accept *:194,"
                             "accept *:220,"
                             "accept *:389,"
                             "accept *:443,"
                             "accept *:464,"
                             "accept *:531,"
                             "accept *:543-544,"
                             "accept *:554,"
                             "accept *:563,"
                             "accept *:636,"
                             "accept *:706,"
                             "accept *:749,"
                             "accept *:873,"
                             "accept *:902-904,"
                             "accept *:981,"
                             "accept *:989-995,"
                             "accept *:1194,"
                             "accept *:1220,"
                             "accept *:1293,"
                             "accept *:1500,"
                             "accept *:1533,"
                             "accept *:1677,"
                             "accept *:1723,"
                             "accept *:1755,"
                             "accept *:1863,"
                             "accept *:2082,"
                             "accept *:2083,"
                             "accept *:2086-2087,"
                             "accept *:2095-2096,"
                             "accept *:2102-2104,"
                             "accept *:3128,"
                             "accept *:3389,"
                             "accept *:3690,"
                             "accept *:4321,"
                             "accept *:4643,"
                             "accept *:5050,"
                             "accept *:5190,"
                             "accept *:5222-5223,"
                             "accept *:5228,"
                             "accept *:5900,"
                             "accept *:6660-6669,"
                             "accept *:6679,"
                             "accept *:6697,"
                             "accept *:8000,"
                             "accept *:8008,"
                             "accept *:8074,"
                             "accept *:8080,"
                             "accept *:8087-8088,"
                             "accept *:8332-8333,"
                             "accept *:8443,"
                             "accept *:8888,"
                             "accept *:9418,"
                             "accept *:9999,"
                             "accept *:10000,"
                             "accept *:11371,"
                             "accept *:12350,"
                             "accept *:19294,"
                             "accept *:19638,"
                             "accept *:23456,"
                             "accept *:33033,"
                             "accept *:64738,"
                             "reject *:*",
                             "accept 20-23,43,53,79-81,88,110,143,194,220,389,"
                             "443,464,531,543-544,554,563,636,706,749,873,"
                             "902-904,981,989-995,1194,1220,1293,1500,1533,"
                             "1677,1723,1755,1863,2082-2083,2086-2087,"
                             "2095-2096,2102-2104,3128,3389,3690,4321,4643,"
                             "5050,5190,5222-5223,5228,5900,6660-6669,6679,"
                             "6697,8000,8008,8074,8080,8087-8088,8332-8333,"
                             "8443,8888,9418,9999-10000,11371,12350,19294,"
                             "19638,23456,33033,64738");
  /* short policy with configured addresses */
  test_policy_summary_helper("reject 149.56.1.1:*,"
                             "reject [2607:5300:1:1::1:0]:*,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "accept 80,443");
  /* short policy with configured and local interface addresses */
  test_policy_summary_helper("reject 149.56.1.0:*,"
                             "reject 149.56.1.1:*,"
                             "reject 149.56.1.2:*,"
                             "reject 149.56.1.3:*,"
                             "reject 149.56.1.4:*,"
                             "reject 149.56.1.5:*,"
                             "reject 149.56.1.6:*,"
                             "reject 149.56.1.7:*,"
                             "reject [2607:5300:1:1::1:0]:*,"
                             "reject [2607:5300:1:1::1:1]:*,"
                             "reject [2607:5300:1:1::1:2]:*,"
                             "reject [2607:5300:1:1::1:3]:*,"
                             "reject [2607:5300:1:1::2:0]:*,"
                             "reject [2607:5300:1:1::2:1]:*,"
                             "reject [2607:5300:1:1::2:2]:*,"
                             "reject [2607:5300:1:1::2:3]:*,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "accept 80,443");
  /* short policy with configured netblocks */
  test_policy_summary_helper("reject 149.56.0.0/16,"
                             "reject6 2607:5300::/32,"
                             "reject6 2608:5300::/64,"
                             "reject6 2609:5300::/96,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "accept 80,443");
  /* short policy with large netblocks that do not count as a rejection */
  test_policy_summary_helper("reject 148.0.0.0/7,"
                             "reject6 2600::/16,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "accept 80,443");
  /* short policy with large netblocks that count as a rejection */
  test_policy_summary_helper("reject 148.0.0.0/6,"
                             "reject6 2600::/15,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "reject 1-65535");
  /* short policy with huge netblocks that count as a rejection */
  test_policy_summary_helper("reject 128.0.0.0/1,"
                             "reject6 8000::/1,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "reject 1-65535");
  /* short policy which blocks everything using netblocks */
  test_policy_summary_helper("reject 0.0.0.0/0,"
                             "reject6 ::/0,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "reject 1-65535");
  /* short policy which has repeated redundant netblocks */
  test_policy_summary_helper("reject 0.0.0.0/0,"
                             "reject 0.0.0.0/0,"
                             "reject 0.0.0.0/0,"
                             "reject 0.0.0.0/0,"
                             "reject 0.0.0.0/0,"
                             "reject6 ::/0,"
                             "reject6 ::/0,"
                             "reject6 ::/0,"
                             "reject6 ::/0,"
                             "reject6 ::/0,"
                             "accept *:80,"
                             "accept *:443,"
                             "reject *:*",
                             "reject 1-65535");

  /* longest possible policy
   * (1-2,4-5,... is longer, but gets reduced to 3,6,... )
   * Going all the way to 65535 is incredibly slow, so we just go slightly
   * more than the expected length */
  test_policy_summary_helper("accept *:1,"
                             "accept *:3,"
                             "accept *:5,"
                             "accept *:7,"
                             "accept *:9,"
                             "accept *:11,"
                             "accept *:13,"
                             "accept *:15,"
                             "accept *:17,"
                             "accept *:19,"
                             "accept *:21,"
                             "accept *:23,"
                             "accept *:25,"
                             "accept *:27,"
                             "accept *:29,"
                             "accept *:31,"
                             "accept *:33,"
                             "accept *:35,"
                             "accept *:37,"
                             "accept *:39,"
                             "accept *:41,"
                             "accept *:43,"
                             "accept *:45,"
                             "accept *:47,"
                             "accept *:49,"
                             "accept *:51,"
                             "accept *:53,"
                             "accept *:55,"
                             "accept *:57,"
                             "accept *:59,"
                             "accept *:61,"
                             "accept *:63,"
                             "accept *:65,"
                             "accept *:67,"
                             "accept *:69,"
                             "accept *:71,"
                             "accept *:73,"
                             "accept *:75,"
                             "accept *:77,"
                             "accept *:79,"
                             "accept *:81,"
                             "accept *:83,"
                             "accept *:85,"
                             "accept *:87,"
                             "accept *:89,"
                             "accept *:91,"
                             "accept *:93,"
                             "accept *:95,"
                             "accept *:97,"
                             "accept *:99,"
                             "accept *:101,"
                             "accept *:103,"
                             "accept *:105,"
                             "accept *:107,"
                             "accept *:109,"
                             "accept *:111,"
                             "accept *:113,"
                             "accept *:115,"
                             "accept *:117,"
                             "accept *:119,"
                             "accept *:121,"
                             "accept *:123,"
                             "accept *:125,"
                             "accept *:127,"
                             "accept *:129,"
                             "accept *:131,"
                             "accept *:133,"
                             "accept *:135,"
                             "accept *:137,"
                             "accept *:139,"
                             "accept *:141,"
                             "accept *:143,"
                             "accept *:145,"
                             "accept *:147,"
                             "accept *:149,"
                             "accept *:151,"
                             "accept *:153,"
                             "accept *:155,"
                             "accept *:157,"
                             "accept *:159,"
                             "accept *:161,"
                             "accept *:163,"
                             "accept *:165,"
                             "accept *:167,"
                             "accept *:169,"
                             "accept *:171,"
                             "accept *:173,"
                             "accept *:175,"
                             "accept *:177,"
                             "accept *:179,"
                             "accept *:181,"
                             "accept *:183,"
                             "accept *:185,"
                             "accept *:187,"
                             "accept *:189,"
                             "accept *:191,"
                             "accept *:193,"
                             "accept *:195,"
                             "accept *:197,"
                             "accept *:199,"
                             "accept *:201,"
                             "accept *:203,"
                             "accept *:205,"
                             "accept *:207,"
                             "accept *:209,"
                             "accept *:211,"
                             "accept *:213,"
                             "accept *:215,"
                             "accept *:217,"
                             "accept *:219,"
                             "accept *:221,"
                             "accept *:223,"
                             "accept *:225,"
                             "accept *:227,"
                             "accept *:229,"
                             "accept *:231,"
                             "accept *:233,"
                             "accept *:235,"
                             "accept *:237,"
                             "accept *:239,"
                             "accept *:241,"
                             "accept *:243,"
                             "accept *:245,"
                             "accept *:247,"
                             "accept *:249,"
                             "accept *:251,"
                             "accept *:253,"
                             "accept *:255,"
                             "accept *:257,"
                             "accept *:259,"
                             "accept *:261,"
                             "accept *:263,"
                             "accept *:265,"
                             "accept *:267,"
                             "accept *:269,"
                             "accept *:271,"
                             "accept *:273,"
                             "accept *:275,"
                             "accept *:277,"
                             "accept *:279,"
                             "accept *:281,"
                             "accept *:283,"
                             "accept *:285,"
                             "accept *:287,"
                             "accept *:289,"
                             "accept *:291,"
                             "accept *:293,"
                             "accept *:295,"
                             "accept *:297,"
                             "accept *:299,"
                             "accept *:301,"
                             "accept *:303,"
                             "accept *:305,"
                             "accept *:307,"
                             "accept *:309,"
                             "accept *:311,"
                             "accept *:313,"
                             "accept *:315,"
                             "accept *:317,"
                             "accept *:319,"
                             "accept *:321,"
                             "accept *:323,"
                             "accept *:325,"
                             "accept *:327,"
                             "accept *:329,"
                             "accept *:331,"
                             "accept *:333,"
                             "accept *:335,"
                             "accept *:337,"
                             "accept *:339,"
                             "accept *:341,"
                             "accept *:343,"
                             "accept *:345,"
                             "accept *:347,"
                             "accept *:349,"
                             "accept *:351,"
                             "accept *:353,"
                             "accept *:355,"
                             "accept *:357,"
                             "accept *:359,"
                             "accept *:361,"
                             "accept *:363,"
                             "accept *:365,"
                             "accept *:367,"
                             "accept *:369,"
                             "accept *:371,"
                             "accept *:373,"
                             "accept *:375,"
                             "accept *:377,"
                             "accept *:379,"
                             "accept *:381,"
                             "accept *:383,"
                             "accept *:385,"
                             "accept *:387,"
                             "accept *:389,"
                             "accept *:391,"
                             "accept *:393,"
                             "accept *:395,"
                             "accept *:397,"
                             "accept *:399,"
                             "accept *:401,"
                             "accept *:403,"
                             "accept *:405,"
                             "accept *:407,"
                             "accept *:409,"
                             "accept *:411,"
                             "accept *:413,"
                             "accept *:415,"
                             "accept *:417,"
                             "accept *:419,"
                             "accept *:421,"
                             "accept *:423,"
                             "accept *:425,"
                             "accept *:427,"
                             "accept *:429,"
                             "accept *:431,"
                             "accept *:433,"
                             "accept *:435,"
                             "accept *:437,"
                             "accept *:439,"
                             "accept *:441,"
                             "accept *:443,"
                             "accept *:445,"
                             "accept *:447,"
                             "accept *:449,"
                             "accept *:451,"
                             "accept *:453,"
                             "accept *:455,"
                             "accept *:457,"
                             "accept *:459,"
                             "accept *:461,"
                             "accept *:463,"
                             "accept *:465,"
                             "accept *:467,"
                             "accept *:469,"
                             "accept *:471,"
                             "accept *:473,"
                             "accept *:475,"
                             "accept *:477,"
                             "accept *:479,"
                             "accept *:481,"
                             "accept *:483,"
                             "accept *:485,"
                             "accept *:487,"
                             "accept *:489,"
                             "accept *:491,"
                             "accept *:493,"
                             "accept *:495,"
                             "accept *:497,"
                             "accept *:499,"
                             "accept *:501,"
                             "accept *:503,"
                             "accept *:505,"
                             "accept *:507,"
                             "accept *:509,"
                             "accept *:511,"
                             "accept *:513,"
                             "accept *:515,"
                             "accept *:517,"
                             "accept *:519,"
                             "accept *:521,"
                             "accept *:523,"
                             "accept *:525,"
                             "accept *:527,"
                             "accept *:529,"
                             "reject *:*",
                             "accept 1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,"
                             "31,33,35,37,39,41,43,45,47,49,51,53,55,57,59,61,"
                             "63,65,67,69,71,73,75,77,79,81,83,85,87,89,91,93,"
                             "95,97,99,101,103,105,107,109,111,113,115,117,"
                             "119,121,123,125,127,129,131,133,135,137,139,141,"
                             "143,145,147,149,151,153,155,157,159,161,163,165,"
                             "167,169,171,173,175,177,179,181,183,185,187,189,"
                             "191,193,195,197,199,201,203,205,207,209,211,213,"
                             "215,217,219,221,223,225,227,229,231,233,235,237,"
                             "239,241,243,245,247,249,251,253,255,257,259,261,"
                             "263,265,267,269,271,273,275,277,279,281,283,285,"
                             "287,289,291,293,295,297,299,301,303,305,307,309,"
                             "311,313,315,317,319,321,323,325,327,329,331,333,"
                             "335,337,339,341,343,345,347,349,351,353,355,357,"
                             "359,361,363,365,367,369,371,373,375,377,379,381,"
                             "383,385,387,389,391,393,395,397,399,401,403,405,"
                             "407,409,411,413,415,417,419,421,423,425,427,429,"
                             "431,433,435,437,439,441,443,445,447,449,451,453,"
                             "455,457,459,461,463,465,467,469,471,473,475,477,"
                             "479,481,483,485,487,489,491,493,495,497,499,501,"
                             "503,505,507,509,511,513,515,517,519,521,523");

  /* Short policies with unrecognized formats should get accepted. */
  test_short_policy_parse("accept fred,2,3-5", "accept 2,3-5");
  test_short_policy_parse("accept 2,fred,3", "accept 2,3");
  test_short_policy_parse("accept 2,fred,3,bob", "accept 2,3");
  test_short_policy_parse("accept 2,-3,500-600", "accept 2,500-600");
  /* Short policies with nil entries are accepted too. */
  test_short_policy_parse("accept 1,,3", "accept 1,3");
  test_short_policy_parse("accept 100-200,,", "accept 100-200");
  test_short_policy_parse("reject ,1-10,,,,30-40", "reject 1-10,30-40");

  /* Try parsing various broken short policies */
#define TT_BAD_SHORT_POLICY(s)                                          \
  do {                                                                  \
    tt_ptr_op(NULL, OP_EQ, (short_parsed = parse_short_policy((s))));      \
  } while (0)
  TT_BAD_SHORT_POLICY("accept 200-199");
  TT_BAD_SHORT_POLICY("");
  TT_BAD_SHORT_POLICY("rejekt 1,2,3");
  TT_BAD_SHORT_POLICY("reject ");
  TT_BAD_SHORT_POLICY("reject");
  TT_BAD_SHORT_POLICY("rej");
  TT_BAD_SHORT_POLICY("accept 2,3,100000");
  TT_BAD_SHORT_POLICY("accept 2,3x,4");
  TT_BAD_SHORT_POLICY("accept 2,3x,4");
  TT_BAD_SHORT_POLICY("accept 2-");
  TT_BAD_SHORT_POLICY("accept 2-x");
  TT_BAD_SHORT_POLICY("accept 1-,3");
  TT_BAD_SHORT_POLICY("accept 1-,3");

  /* Make sure that IPv4 addresses are ignored in accept6/reject6 lines. */
  p = router_parse_addr_policy_item_from_string("accept6 1.2.3.4:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("reject6 2.4.6.0/24:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(!malformed_list);

  p = router_parse_addr_policy_item_from_string("accept6 *4:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(!malformed_list);

  /* Make sure malformed policies are detected as such. */
  p = router_parse_addr_policy_item_from_string("bad_token *4:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("accept6 **:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("accept */15:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("reject6 */:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("accept 127.0.0.1/33:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("accept6 [::1]/129:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("reject 8.8.8.8/-1:*", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("reject 8.8.4.4:10-5", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  p = router_parse_addr_policy_item_from_string("reject 1.2.3.4:-1", -1,
                                                &malformed_list);
  tt_assert(p == NULL);
  tt_assert(malformed_list);

  /* Test a too-long policy. */
  {
    char *policy_strng = NULL;
    smartlist_t *chunks = smartlist_new();
    smartlist_add(chunks, tor_strdup("accept "));
    for (i=1; i<10000; ++i)
      smartlist_add_asprintf(chunks, "%d,", i);
    smartlist_add(chunks, tor_strdup("20000"));
    policy_strng = smartlist_join_strings(chunks, "", 0, NULL);
    SMARTLIST_FOREACH(chunks, char *, ch, tor_free(ch));
    smartlist_free(chunks);
    short_parsed = parse_short_policy(policy_strng);/* shouldn't be accepted */
    tor_free(policy_strng);
    tt_ptr_op(NULL, OP_EQ, short_parsed);
  }

  /* truncation ports */
  sm = smartlist_new();
  for (i=1; i<2000; i+=2) {
    char buf[POLICY_BUF_LEN];
    tor_snprintf(buf, sizeof(buf), "reject *:%d", i);
    smartlist_add(sm, tor_strdup(buf));
  }
  smartlist_add(sm, tor_strdup("accept *:*"));
  policy_str = smartlist_join_strings(sm, ",", 0, NULL);
  test_policy_summary_helper( policy_str,
    "accept 2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,"
    "46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,"
    "92,94,96,98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,"
    "130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,"
    "166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,"
    "202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,236,"
    "238,240,242,244,246,248,250,252,254,256,258,260,262,264,266,268,270,272,"
    "274,276,278,280,282,284,286,288,290,292,294,296,298,300,302,304,306,308,"
    "310,312,314,316,318,320,322,324,326,328,330,332,334,336,338,340,342,344,"
    "346,348,350,352,354,356,358,360,362,364,366,368,370,372,374,376,378,380,"
    "382,384,386,388,390,392,394,396,398,400,402,404,406,408,410,412,414,416,"
    "418,420,422,424,426,428,430,432,434,436,438,440,442,444,446,448,450,452,"
    "454,456,458,460,462,464,466,468,470,472,474,476,478,480,482,484,486,488,"
    "490,492,494,496,498,500,502,504,506,508,510,512,514,516,518,520,522");

 done:
  addr_policy_list_free(policy);
  addr_policy_list_free(policy2);
  addr_policy_list_free(policy3);
  addr_policy_list_free(policy4);
  addr_policy_list_free(policy5);
  addr_policy_list_free(policy6);
  addr_policy_list_free(policy7);
  addr_policy_list_free(policy8);
  addr_policy_list_free(policy9);
  addr_policy_list_free(policy10);
  addr_policy_list_free(policy11);
  addr_policy_list_free(policy12);
  tor_free(policy_str);
  if (sm) {
    SMARTLIST_FOREACH(sm, char *, s, tor_free(s));
    smartlist_free(sm);
  }
  short_policy_free(short_parsed);
}
int main(void) {
        test_syslog_parse_identifier("pidu[111]: xxx", "pidu", "111", 11);
        test_syslog_parse_identifier("pidu: xxx", "pidu", NULL, 6);
        test_syslog_parse_identifier("pidu:  xxx", "pidu", NULL, 7);
        test_syslog_parse_identifier("pidu xxx", NULL, NULL, 0);
        test_syslog_parse_identifier(":", "", NULL, 1);
        test_syslog_parse_identifier(":  ", "", NULL, 3);
        test_syslog_parse_identifier("pidu:", "pidu", NULL, 5);
        test_syslog_parse_identifier("pidu: ", "pidu", NULL, 6);
        test_syslog_parse_identifier("pidu : ", NULL, NULL, 0);

        test_syslog_parse_priority("<>", 0, 0);
        test_syslog_parse_priority("<>aaa", 0, 0);
        test_syslog_parse_priority("<aaaa>", 0, 0);
        test_syslog_parse_priority("<aaaa>aaa", 0, 0);
        test_syslog_parse_priority(" <aaaa>", 0, 0);
        test_syslog_parse_priority(" <aaaa>aaa", 0, 0);
        /* TODO: add test cases of valid priorities */

        return 0;
}
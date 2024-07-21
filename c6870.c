int main(void) {
        test_syslog_parse_identifier("pidu[111]: xxx", "pidu", "111", 11);
        test_syslog_parse_identifier("pidu: xxx", "pidu", NULL, 6);
        test_syslog_parse_identifier("pidu xxx", NULL, NULL, 0);

        return 0;
}
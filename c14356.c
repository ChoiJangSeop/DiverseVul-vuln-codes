struct razer_report razer_chroma_misc_one_row_set_custom_frame(unsigned char start_col, unsigned char stop_col, unsigned char *rgb_data) // TODO recheck custom frame hex
{
    struct razer_report report = get_razer_report(0x03, 0x0C, 0x32);
    size_t row_length = (size_t) (((stop_col + 1) - start_col) * 3);

    report.arguments[0] = start_col;
    report.arguments[1] = stop_col;

    memcpy(&report.arguments[2], rgb_data, row_length);

    return report;
}
int main(int argc, char **argv)
{
    const char *arch = qtest_get_arch();

    g_test_init(&argc, &argv, NULL);

    if (strcmp(arch, "i386") == 0 || strcmp(arch, "x86_64") == 0) {
        qtest_add_func("fuzz/test_lp1878263_megasas_zero_iov_cnt",
                       test_lp1878263_megasas_zero_iov_cnt);
        qtest_add_func("fuzz/test_lp1878642_pci_bus_get_irq_level_assert",
                       test_lp1878642_pci_bus_get_irq_level_assert);
    }

    return g_test_run();
}
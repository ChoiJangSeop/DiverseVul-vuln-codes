bj10v_open(gx_device * pdev)
{
    if (pdev->HWResolution[0] < 180 ||
        pdev->HWResolution[1] < 180)
    {
        emprintf("device requires a resolution of at least 180dpi\n");
        return_error(gs_error_rangecheck);
    }
    return gdev_prn_open(pdev);
}
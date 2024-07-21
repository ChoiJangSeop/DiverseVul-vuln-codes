md_open(gx_device *pdev)
{
        static const float md_margins[4] =
         {	MD_SIDE_MARGIN, MD_BOTTOM_MARGIN,
                MD_SIDE_MARGIN, MD_TOP_MARGIN
         };

        gx_device_set_margins(pdev, md_margins, true);
        return gdev_prn_open(pdev);
}
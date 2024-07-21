pdf14_dev_spec_op(gx_device *pdev, int dev_spec_op,
                  void *data, int size)
{
    pdf14_device * p14dev = (pdf14_device *)pdev;

    if (dev_spec_op == gxdso_pattern_shfill_doesnt_need_path)
        return 1;
    if (dev_spec_op == gxdso_is_pdf14_device) {
        if (data != NULL && size == sizeof(gx_device *))
            *(gx_device **)data = pdev;
        return 1;
    }
    if (dev_spec_op == gxdso_device_child) {
        pdf14_device *dev = (pdf14_device *)pdev;
        gxdso_device_child_request *d = (gxdso_device_child_request *)data;
        if (d->target == pdev) {
            d->target = dev->target;
            return 1;
        }
    }
    if (dev_spec_op == gxdso_supports_devn) {
        cmm_dev_profile_t *dev_profile;
        int code;
        code = dev_proc(pdev, get_profile)((gx_device*) pdev, &dev_profile);
        if (code == 0) {
            return dev_profile->supports_devn;
        } else {
            return 0;
        }
    }
    if (dev_spec_op == gxdso_get_dev_param || dev_spec_op == gxdso_restrict_bbox) {
        return dev_proc(p14dev->target, dev_spec_op)(p14dev->target, dev_spec_op, data, size);
    }

    return gx_default_dev_spec_op(pdev, dev_spec_op, data, size);

}
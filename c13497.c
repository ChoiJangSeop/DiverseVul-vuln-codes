int udf_get_filename(struct super_block *sb, uint8_t *sname, uint8_t *dname,
		     int flen)
{
	struct ustr *filename, *unifilename;
	int len = 0;

	filename = kmalloc(sizeof(struct ustr), GFP_NOFS);
	if (!filename)
		return 0;

	unifilename = kmalloc(sizeof(struct ustr), GFP_NOFS);
	if (!unifilename)
		goto out1;

	if (udf_build_ustr_exact(unifilename, sname, flen))
		goto out2;

	if (UDF_QUERY_FLAG(sb, UDF_FLAG_UTF8)) {
		if (!udf_CS0toUTF8(filename, unifilename)) {
			udf_debug("Failed in udf_get_filename: sname = %s\n",
				  sname);
			goto out2;
		}
	} else if (UDF_QUERY_FLAG(sb, UDF_FLAG_NLS_MAP)) {
		if (!udf_CS0toNLS(UDF_SB(sb)->s_nls_map, filename,
				  unifilename)) {
			udf_debug("Failed in udf_get_filename: sname = %s\n",
				  sname);
			goto out2;
		}
	} else
		goto out2;

	len = udf_translate_to_linux(dname, filename->u_name, filename->u_len,
				     unifilename->u_name, unifilename->u_len);
out2:
	kfree(unifilename);
out1:
	kfree(filename);
	return len;
}
static Suite *virgl_init_suite(void)
{
  Suite *s;
  TCase *tc_core;

  s = suite_create("virgl_transfer");
  tc_core = tcase_create("transfer_direct");

  tcase_add_checked_fixture(tc_core, testvirgl_init_single_ctx_nr, testvirgl_fini_single_ctx);
  tcase_add_test(tc_core, virgl_test_transfer_read_illegal_ctx);
  tcase_add_test(tc_core, virgl_test_transfer_write_illegal_ctx);
  tcase_add_test(tc_core, virgl_test_transfer_read_unbound_res);
  tcase_add_test(tc_core, virgl_test_transfer_write_unbound_res);
  tcase_add_test(tc_core, virgl_test_transfer_read_no_iov);
  tcase_add_test(tc_core, virgl_test_transfer_write_no_iov);
  tcase_add_test(tc_core, virgl_test_transfer_read_no_box);
  tcase_add_test(tc_core, virgl_test_transfer_write_no_box);
  tcase_add_test(tc_core, virgl_test_transfer_read_1d_bad_box);
  tcase_add_test(tc_core, virgl_test_transfer_write_1d_bad_box);
  tcase_add_test(tc_core, virgl_test_transfer_read_1d_array_bad_box);
  tcase_add_test(tc_core, virgl_test_transfer_read_3d_bad_box);
  tcase_add_test(tc_core, virgl_test_transfer_1d);
  tcase_add_test(tc_core, virgl_test_transfer_1d_bad_iov);
  tcase_add_test(tc_core, virgl_test_transfer_1d_bad_iov_offset);
  tcase_add_test(tc_core, virgl_test_transfer_1d_bad_strides);
  tcase_add_test(tc_core, virgl_test_transfer_2d_bad_strides);
  tcase_add_test(tc_core, virgl_test_transfer_buffer_bad_strides);
  tcase_add_test(tc_core, virgl_test_transfer_2d_array_bad_layer_stride);
  tcase_add_test(tc_core, virgl_test_transfer_2d_bad_level);

  tcase_add_loop_test(tc_core, virgl_test_transfer_res_read_valid, 0, PIPE_MAX_TEXTURE_TYPES);
  tcase_add_loop_test(tc_core, virgl_test_transfer_res_write_valid, 0, PIPE_MAX_TEXTURE_TYPES);
  tcase_add_loop_test(tc_core, virgl_test_transfer_res_read_invalid, 0, PIPE_MAX_TEXTURE_TYPES);
  tcase_add_loop_test(tc_core, virgl_test_transfer_res_write_invalid, 0, PIPE_MAX_TEXTURE_TYPES);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("transfer_inline_write");
  tcase_add_loop_test(tc_core, virgl_test_transfer_inline_valid, 0, PIPE_MAX_TEXTURE_TYPES);
  tcase_add_loop_test(tc_core, virgl_test_transfer_inline_invalid, 0, PIPE_MAX_TEXTURE_TYPES);
  tcase_add_loop_test(tc_core, virgl_test_transfer_inline_valid_large, 0, PIPE_MAX_TEXTURE_TYPES);

  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("transfers_staging");
  tcase_add_test(tc_core, virgl_test_transfer_to_staging_without_iov_fails);
  tcase_add_test(tc_core, virgl_test_transfer_to_staging_with_iov_succeeds);
  tcase_add_test(tc_core, virgl_test_copy_transfer_from_staging_without_iov_fails);
  tcase_add_test(tc_core, virgl_test_copy_transfer_from_staging_with_iov_succeeds);
  tcase_add_test(tc_core, virgl_test_copy_transfer_to_staging_without_iov_fails);
  tcase_add_test(tc_core, virgl_test_copy_transfer_to_staging_with_iov_succeeds);

  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("transfer_command_bounds");
  tcase_add_test(tc_core, virgl_test_transfer_near_res_bounds_with_stride_succeeds);

  suite_add_tcase(s, tc_core);

  return s;

}
finished_read (void *vp, int *error)
{
  struct command *command = vp;

  if (allocated || sparse_size == 0) {
    /* If sparseness detection (see below) is turned off then we write
     * the whole command.
     */
    dst->ops->asynch_write (dst, command,
                            (nbd_completion_callback) {
                              .callback = free_command,
                              .user_data = command,
                            });
  }
  else {                               /* Sparseness detection. */
    const uint64_t start = command->offset;
    const uint64_t end = start + command->slice.len;
    uint64_t last_offset = start;
    bool last_is_hole = false;
    uint64_t i;
    struct command *newcommand;
    int dummy = 0;

    /* Iterate over whole blocks in the command, starting on a block
     * boundary.
     */
    for (i = MIN (ROUND_UP (start, sparse_size), end);
         i + sparse_size <= end;
         i += sparse_size) {
      if (is_zero (slice_ptr (command->slice) + i-start, sparse_size)) {
        /* It's a hole.  If the last was a hole too then we do nothing
         * here which coalesces.  Otherwise write the last data and
         * start a new hole.
         */
        if (!last_is_hole) {
          /* Write the last data (if any). */
          if (i - last_offset > 0) {
            newcommand = copy_subcommand (command,
                                          last_offset, i - last_offset,
                                          false);
            dst->ops->asynch_write (dst, newcommand,
                                    (nbd_completion_callback) {
                                      .callback = free_command,
                                      .user_data = newcommand,
                                    });
          }
          /* Start the new hole. */
          last_offset = i;
          last_is_hole = true;
        }
      }
      else {
        /* It's data.  If the last was data too, do nothing =>
         * coalesce.  Otherwise write the last hole and start a new
         * data.
         */
        if (last_is_hole) {
          /* Write the last hole (if any). */
          if (i - last_offset > 0) {
            newcommand = copy_subcommand (command,
                                          last_offset, i - last_offset,
                                          true);
            fill_dst_range_with_zeroes (newcommand);
          }
          /* Start the new data. */
          last_offset = i;
          last_is_hole = false;
        }
      }
    } /* for i */

    /* Write the last_offset up to i. */
    if (i - last_offset > 0) {
      if (!last_is_hole) {
        newcommand = copy_subcommand (command,
                                      last_offset, i - last_offset,
                                      false);
        dst->ops->asynch_write (dst, newcommand,
                                (nbd_completion_callback) {
                                  .callback = free_command,
                                  .user_data = newcommand,
                                });
      }
      else {
        newcommand = copy_subcommand (command,
                                      last_offset, i - last_offset,
                                      true);
        fill_dst_range_with_zeroes (newcommand);
      }
    }

    /* There may be an unaligned tail, so write that. */
    if (end - i > 0) {
      newcommand = copy_subcommand (command, i, end - i, false);
      dst->ops->asynch_write (dst, newcommand,
                              (nbd_completion_callback) {
                                .callback = free_command,
                                .user_data = newcommand,
                              });
    }

    /* Free the original command since it has been split into
     * subcommands and the original is no longer needed.
     */
    free_command (command, &dummy);
  }

  return 1; /* auto-retires the command */
}
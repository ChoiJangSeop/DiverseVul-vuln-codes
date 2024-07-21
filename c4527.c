GC_allochblk_nth(size_t sz, int kind, unsigned flags, int n, int may_split)
{
    struct hblk *hbp;
    hdr * hhdr;                 /* Header corr. to hbp */
    struct hblk *thishbp;
    hdr * thishdr;              /* Header corr. to thishbp */
    signed_word size_needed;    /* number of bytes in requested objects */
    signed_word size_avail;     /* bytes available in this block        */

    size_needed = HBLKSIZE * OBJ_SZ_TO_BLOCKS(sz);

    /* search for a big enough block in free list */
        for (hbp = GC_hblkfreelist[n];; hbp = hhdr -> hb_next) {
            if (NULL == hbp) return NULL;
            GET_HDR(hbp, hhdr); /* set hhdr value */
            size_avail = hhdr->hb_sz;
            if (size_avail < size_needed) continue;
            if (size_avail != size_needed) {
              signed_word next_size;

              if (!may_split) continue;
              /* If the next heap block is obviously better, go on.     */
              /* This prevents us from disassembling a single large     */
              /* block to get tiny blocks.                              */
              thishbp = hhdr -> hb_next;
              if (thishbp != 0) {
                GET_HDR(thishbp, thishdr);
                next_size = (signed_word)(thishdr -> hb_sz);
                if (next_size < size_avail
                    && next_size >= size_needed
                    && !GC_is_black_listed(thishbp, (word)size_needed)) {
                    continue;
                }
              }
            }
            if (!IS_UNCOLLECTABLE(kind) && (kind != PTRFREE
                        || size_needed > (signed_word)MAX_BLACK_LIST_ALLOC)) {
              struct hblk * lasthbp = hbp;
              ptr_t search_end = (ptr_t)hbp + size_avail - size_needed;
              signed_word orig_avail = size_avail;
              signed_word eff_size_needed = (flags & IGNORE_OFF_PAGE) != 0 ?
                                                (signed_word)HBLKSIZE
                                                : size_needed;

              while ((word)lasthbp <= (word)search_end
                     && (thishbp = GC_is_black_listed(lasthbp,
                                            (word)eff_size_needed)) != 0) {
                lasthbp = thishbp;
              }
              size_avail -= (ptr_t)lasthbp - (ptr_t)hbp;
              thishbp = lasthbp;
              if (size_avail >= size_needed) {
                if (thishbp != hbp) {
#                 ifdef USE_MUNMAP
                    /* Avoid remapping followed by splitting.   */
                    if (may_split == AVOID_SPLIT_REMAPPED && !IS_MAPPED(hhdr))
                      continue;
#                 endif
                  thishdr = GC_install_header(thishbp);
                  if (0 != thishdr) {
                  /* Make sure it's mapped before we mangle it. */
#                   ifdef USE_MUNMAP
                      if (!IS_MAPPED(hhdr)) {
                        GC_remap((ptr_t)hbp, hhdr -> hb_sz);
                        hhdr -> hb_flags &= ~WAS_UNMAPPED;
                      }
#                   endif
                  /* Split the block at thishbp */
                      GC_split_block(hbp, hhdr, thishbp, thishdr, n);
                  /* Advance to thishbp */
                      hbp = thishbp;
                      hhdr = thishdr;
                      /* We must now allocate thishbp, since it may     */
                      /* be on the wrong free list.                     */
                  }
                }
              } else if (size_needed > (signed_word)BL_LIMIT
                         && orig_avail - size_needed
                            > (signed_word)BL_LIMIT) {
                /* Punt, since anything else risks unreasonable heap growth. */
                if (++GC_large_alloc_warn_suppressed
                    >= GC_large_alloc_warn_interval) {
                  WARN("Repeated allocation of very large block "
                       "(appr. size %" WARN_PRIdPTR "):\n"
                       "\tMay lead to memory leak and poor performance\n",
                       size_needed);
                  GC_large_alloc_warn_suppressed = 0;
                }
                size_avail = orig_avail;
              } else if (size_avail == 0 && size_needed == HBLKSIZE
                         && IS_MAPPED(hhdr)) {
                if (!GC_find_leak) {
                  static unsigned count = 0;

                  /* The block is completely blacklisted.  We need      */
                  /* to drop some such blocks, since otherwise we spend */
                  /* all our time traversing them if pointer-free       */
                  /* blocks are unpopular.                              */
                  /* A dropped block will be reconsidered at next GC.   */
                  if ((++count & 3) == 0) {
                    /* Allocate and drop the block in small chunks, to  */
                    /* maximize the chance that we will recover some    */
                    /* later.                                           */
                      word total_size = hhdr -> hb_sz;
                      struct hblk * limit = hbp + divHBLKSZ(total_size);
                      struct hblk * h;
                      struct hblk * prev = hhdr -> hb_prev;

                      GC_large_free_bytes -= total_size;
                      GC_bytes_dropped += total_size;
                      GC_remove_from_fl_at(hhdr, n);
                      for (h = hbp; (word)h < (word)limit; h++) {
                        if (h != hbp) {
                          hhdr = GC_install_header(h);
                        }
                        if (NULL != hhdr) {
                          (void)setup_header(hhdr, h, HBLKSIZE, PTRFREE, 0);
                                                    /* Can't fail. */
                          if (GC_debugging_started) {
                            BZERO(h, HBLKSIZE);
                          }
                        }
                      }
                    /* Restore hbp to point at free block */
                      hbp = prev;
                      if (0 == hbp) {
                        return GC_allochblk_nth(sz, kind, flags, n, may_split);
                      }
                      hhdr = HDR(hbp);
                  }
                }
              }
            }
            if( size_avail >= size_needed ) {
#               ifdef USE_MUNMAP
                  if (!IS_MAPPED(hhdr)) {
                    GC_remap((ptr_t)hbp, hhdr -> hb_sz);
                    hhdr -> hb_flags &= ~WAS_UNMAPPED;
                    /* Note: This may leave adjacent, mapped free blocks. */
                  }
#               endif
                /* hbp may be on the wrong freelist; the parameter n    */
                /* is important.                                        */
                hbp = GC_get_first_part(hbp, hhdr, size_needed, n);
                break;
            }
        }

    if (0 == hbp) return 0;

    /* Add it to map of valid blocks */
        if (!GC_install_counts(hbp, (word)size_needed)) return(0);
        /* This leaks memory under very rare conditions. */

    /* Set up header */
        if (!setup_header(hhdr, hbp, sz, kind, flags)) {
            GC_remove_counts(hbp, (word)size_needed);
            return(0); /* ditto */
        }
#   ifndef GC_DISABLE_INCREMENTAL
        /* Notify virtual dirty bit implementation that we are about to */
        /* write.  Ensure that pointer-free objects are not protected   */
        /* if it is avoidable.  This also ensures that newly allocated  */
        /* blocks are treated as dirty.  Necessary since we don't       */
        /* protect free blocks.                                         */
        GC_ASSERT((size_needed & (HBLKSIZE-1)) == 0);
        GC_remove_protection(hbp, divHBLKSZ(size_needed),
                             (hhdr -> hb_descr == 0) /* pointer-free */);
#   endif
    /* We just successfully allocated a block.  Restart count of        */
    /* consecutive failures.                                            */
    GC_fail_count = 0;

    GC_large_free_bytes -= size_needed;
    GC_ASSERT(IS_MAPPED(hhdr));
    return( hbp );
}
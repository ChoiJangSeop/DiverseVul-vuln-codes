int seccomp_memory_deny_write_execute(void) {

        uint32_t arch;
        int r;

        SECCOMP_FOREACH_LOCAL_ARCH(arch) {
                _cleanup_(seccomp_releasep) scmp_filter_ctx seccomp = NULL;
                int filter_syscall = 0, block_syscall = 0, shmat_syscall = 0;

                log_debug("Operating on architecture: %s", seccomp_arch_to_string(arch));

                switch (arch) {

                case SCMP_ARCH_X86:
                        filter_syscall = SCMP_SYS(mmap2);
                        block_syscall = SCMP_SYS(mmap);
                        break;

                case SCMP_ARCH_PPC64:
                case SCMP_ARCH_PPC64LE:
                        filter_syscall = SCMP_SYS(mmap);

                        /* Note that shmat() isn't available, and the call is multiplexed through ipc().
                         * We ignore that here, which means there's still a way to get writable/executable
                         * memory, if an IPC key is mapped like this. That's a pity, but no total loss. */

                        break;

                case SCMP_ARCH_ARM:
                        filter_syscall = SCMP_SYS(mmap2); /* arm has only mmap2 */
                        shmat_syscall = SCMP_SYS(shmat);
                        break;

                case SCMP_ARCH_X86_64:
                case SCMP_ARCH_X32:
                case SCMP_ARCH_AARCH64:
                        filter_syscall = SCMP_SYS(mmap); /* amd64, x32, and arm64 have only mmap */
                        shmat_syscall = SCMP_SYS(shmat);
                        break;

                /* Please add more definitions here, if you port systemd to other architectures! */

#if !defined(__i386__) && !defined(__x86_64__) && !defined(__powerpc64__) && !defined(__arm__) && !defined(__aarch64__)
#warning "Consider adding the right mmap() syscall definitions here!"
#endif
                }

                /* Can't filter mmap() on this arch, then skip it */
                if (filter_syscall == 0)
                        continue;

                r = seccomp_init_for_arch(&seccomp, arch, SCMP_ACT_ALLOW);
                if (r < 0)
                        return r;

                r = add_seccomp_syscall_filter(seccomp, arch, filter_syscall,
                                               1,
                                               SCMP_A2(SCMP_CMP_MASKED_EQ, PROT_EXEC|PROT_WRITE, PROT_EXEC|PROT_WRITE));
                if (r < 0)
                        continue;

                if (block_syscall != 0) {
                        r = add_seccomp_syscall_filter(seccomp, arch, block_syscall, 0, (const struct scmp_arg_cmp){} );
                        if (r < 0)
                                continue;
                }

                r = add_seccomp_syscall_filter(seccomp, arch, SCMP_SYS(mprotect),
                                               1,
                                               SCMP_A2(SCMP_CMP_MASKED_EQ, PROT_EXEC, PROT_EXEC));
                if (r < 0)
                        continue;

                if (shmat_syscall != 0) {
                        r = add_seccomp_syscall_filter(seccomp, arch, SCMP_SYS(shmat),
                                                       1,
                                                       SCMP_A2(SCMP_CMP_MASKED_EQ, SHM_EXEC, SHM_EXEC));
                        if (r < 0)
                                continue;
                }

                r = seccomp_load(seccomp);
                if (IN_SET(r, -EPERM, -EACCES))
                        return r;
                if (r < 0)
                        log_debug_errno(r, "Failed to install MemoryDenyWriteExecute= rule for architecture %s, skipping: %m", seccomp_arch_to_string(arch));
        }

        return 0;
}
Elf32_Sym const *PackLinuxElf32::elf_lookup(char const *name) const
{
    if (hashtab && dynsym && dynstr) {
        unsigned const nbucket = get_te32(&hashtab[0]);
        unsigned const *const buckets = &hashtab[2];
        unsigned const *const chains = &buckets[nbucket];
        unsigned const m = elf_hash(name) % nbucket;
        if (!nbucket
        ||  (unsigned)(file_size - ((char const *)buckets - (char const *)(void const *)file_image))
                <= sizeof(unsigned)*nbucket ) {
            char msg[80]; snprintf(msg, sizeof(msg),
                "bad nbucket %#x\n", nbucket);
            throwCantPack(msg);
        }
        unsigned si;
        for (si= get_te32(&buckets[m]); 0!=si; si= get_te32(&chains[si])) {
            char const *const p= get_dynsym_name(si, (unsigned)-1);
            if (0==strcmp(name, p)) {
                return &dynsym[si];
            }
        }
    }
    if (gashtab && dynsym && dynstr) {
        unsigned const n_bucket = get_te32(&gashtab[0]);
        unsigned const symbias  = get_te32(&gashtab[1]);
        unsigned const n_bitmask = get_te32(&gashtab[2]);
        unsigned const gnu_shift = get_te32(&gashtab[3]);
        unsigned const *const bitmask = &gashtab[4];
        unsigned const *const buckets = &bitmask[n_bitmask];
        unsigned const *const hasharr = &buckets[n_bucket];
        if (!n_bucket
        || (void const *)&file_image[file_size] <= (void const *)hasharr) {
            char msg[80]; snprintf(msg, sizeof(msg),
                "bad n_bucket %#x\n", n_bucket);
            throwCantPack(msg);
        }
        if (!n_bitmask
        || (unsigned)(file_size - ((char const *)bitmask - (char const *)(void const *)file_image))
                <= sizeof(unsigned)*n_bitmask ) {
            char msg[80]; snprintf(msg, sizeof(msg),
                "bad n_bitmask %#x\n", n_bitmask);
            throwCantPack(msg);
        }

        unsigned const h = gnu_hash(name);
        unsigned const hbit1 = 037& h;
        unsigned const hbit2 = 037& (h>>gnu_shift);
        unsigned const w = get_te32(&bitmask[(n_bitmask -1) & (h>>5)]);

        if (1& (w>>hbit1) & (w>>hbit2)) {
            unsigned bucket = get_te32(&buckets[h % n_bucket]);
            if (n_bucket <= bucket) {
                char msg[80]; snprintf(msg, sizeof(msg),
                        "bad DT_GNU_HASH n_bucket{%#x} <= buckets[%d]{%#x}\n",
                        n_bucket, h % n_bucket, bucket);
                throwCantPack(msg);
            }
            if (0!=bucket) {
                Elf32_Sym const *dsp = &dynsym[bucket];
                unsigned const *hp = &hasharr[bucket - symbias];

                do if (0==((h ^ get_te32(hp))>>1)) {
                    unsigned st_name = get_te32(&dsp->st_name);
                    char const *const p = get_str_name(st_name, (unsigned)-1);
                    if (0==strcmp(name, p)) {
                        return dsp;
                    }
                } while (++dsp,
                        (char const *)hp < (char const *)&file_image[file_size]
                    &&  0==(1u& get_te32(hp++)));
            }
        }
    }
    return 0;

}
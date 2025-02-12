zzip_mem_disk_entry_new(ZZIP_DISK* disk, ZZIP_DISK_ENTRY* entry) 
{
    ZZIP_MEM_DISK_ENTRY* item = calloc(1, sizeof(*item));
    struct zzip_file_header* header = 
	zzip_disk_entry_to_file_header(disk, entry);
    /*  there is a number of duplicated information in the file header
     *  or the disk entry block. Theoretically some part may be missing
     *  that exists in the other, so each and every item would need to
     *  be checked. However, we assume that the "always-exists" fields
     *  do either (a) exist both and have the same value or (b) the bits
     *  in the disk entry are correct. Only the variable fields are 
     *  checked in both places: file name, file comment, extra blocks.
     *  From mmapped.c we do already have two helper functions for that:
     */
    item->zz_comment =   zzip_disk_entry_strdup_comment(disk, entry);
    item->zz_name =      zzip_disk_entry_strdup_name(disk, entry);
    item->zz_data =      zzip_file_header_to_data(header);
    item->zz_flags =     zzip_disk_entry_get_flags(entry);
    item->zz_compr =     zzip_disk_entry_get_compr(entry);
    item->zz_crc32 =     zzip_disk_entry_get_crc32(entry);
    item->zz_csize =     zzip_disk_entry_get_csize(entry);
    item->zz_usize =     zzip_disk_entry_get_usize(entry);
    item->zz_diskstart = zzip_disk_entry_get_diskstart(entry);
    item->zz_filetype =  zzip_disk_entry_get_filetype(entry);

    { /* 1. scanning the extra blocks and building a fast-access table. */
	size_t extcount = 0;
	int    extlen = zzip_file_header_get_extras(header);
	char*  extras = zzip_file_header_to_extras(header);
	while (extlen > 0) {
	    struct zzip_extra_block* ext = (struct zzip_extra_block*) extras;
	    int size = zzip_extra_block_sizeto_end(ext);
	    extlen -= size; extras += size; extcount ++;
	}
	extlen = zzip_disk_entry_get_extras(entry);
	extras = zzip_disk_entry_to_extras(entry);
	while (extlen > 0) {
	    struct zzip_extra_block* ext = (struct zzip_extra_block*) extras;
	    int size = zzip_extra_block_sizeto_end(ext);
	    extlen -= size; extras += size; extcount ++;
	}
	if (item->zz_extras) free(item->zz_extras);
	item->zz_extras = calloc(extcount,sizeof(struct _zzip_mem_disk_extra));
	item->zz_extcount = extcount;
    }
    { /* 2. reading the extra blocks and building a fast-access table. */
	size_t ext = 0;
	int    extlen = zzip_file_header_get_extras(header);
	char*  extras = zzip_file_header_to_extras(header);
	struct _zzip_mem_disk_extra* mem = item->zz_extras;
	while (extlen > 0) {
	    struct zzip_extra_block* ext = (struct zzip_extra_block*) extras;
	    mem[ext].zz_data = extras;
	    mem[ext].zz_datatype = zzip_extra_block_get_datatype(ext);
	    mem[ext].zz_datasize = zzip_extra_block_get_datasize(ext);
	    ___ register int size = zzip_extra_block_sizeto_end(ext);
	    extlen -= size; extras += size; ext ++; ____;
	}
	extlen = zzip_disk_entry_get_extras(entry);
	extras = zzip_disk_entry_to_extras(entry);
	while (extlen > 0) {
	    struct zzip_extra_block* ext = (struct zzip_extra_block*) extras;
	    mem[ext].zz_data = extras;
	    mem[ext].zz_datatype = zzip_extra_block_get_datatype(ext);
	    mem[ext].zz_datasize = zzip_extra_block_get_datasize(ext);
	    ___ register int size = zzip_extra_block_sizeto_end(ext);
	    extlen -= size; extras += size; ext ++; ____;
	}
    }
    { /* 3. scanning the extra blocks for platform specific extensions. */
	register size_t ext;
	for (ext = 0; ext < item->zz_extcount; ext++) {
	    /* "http://www.pkware.com/company/standards/appnote/" */
	    switch (item->zz_extras[ext].zz_datatype) {
	    case 0x0001: { /* ZIP64 extended information extra field */
		struct {
		    char z_datatype[2]; /* Tag for this "extra" block type */
		    char z_datasize[2]; /* Size of this "extra" block */
		    char z_usize[8]; /* Original uncompressed file size */
		    char z_csize[8]; /* Size of compressed data */
		    char z_offset[8]; /* Offset of local header record */
		    char z_diskstart[4]; /* Number of the disk for file start*/
		} *block = (void*) item->zz_extras[ext].zz_data;
		item->zz_usize  =    __zzip_get64(block->z_usize);
		item->zz_csize  =    __zzip_get64(block->z_csize);
		item->zz_offset =    __zzip_get64(block->z_offset);
		item->zz_diskstart = __zzip_get32(block->z_diskstart);
	    } break;
	    case 0x0007: /* AV Info */
	    case 0x0008: /* Reserved for future Unicode file name data (PFS) */
	    case 0x0009: /* OS/2 */
	    case 0x000a: /* NTFS */
	    case 0x000c: /* OpenVMS */
	    case 0x000d: /* Unix */
	    case 0x000e: /* Reserved for file stream and fork descriptors */
	    case 0x000f: /* Patch Descriptor */
	    case 0x0014: /* PKCS#7 Store for X.509 Certificates */
	    case 0x0015: /* X.509 Certificate ID and Signature for file */
	    case 0x0016: /* X.509 Certificate ID for Central Directory */
	    case 0x0017: /* Strong Encryption Header */
	    case 0x0018: /* Record Management Controls */
	    case 0x0019: /* PKCS#7 Encryption Recipient Certificate List */
	    case 0x0065: /* IBM S/390, AS/400 attributes - uncompressed */
	    case 0x0066: /* Reserved for IBM S/390, AS/400 attr - compressed */
	    case 0x07c8: /* Macintosh */
	    case 0x2605: /* ZipIt Macintosh */
	    case 0x2705: /* ZipIt Macintosh 1.3.5+ */
	    case 0x2805: /* ZipIt Macintosh 1.3.5+ */
	    case 0x334d: /* Info-ZIP Macintosh */
	    case 0x4341: /* Acorn/SparkFS  */
	    case 0x4453: /* Windows NT security descriptor (binary ACL) */
	    case 0x4704: /* VM/CMS */
	    case 0x470f: /* MVS */
	    case 0x4b46: /* FWKCS MD5 (see below) */
	    case 0x4c41: /* OS/2 access control list (text ACL) */
	    case 0x4d49: /* Info-ZIP OpenVMS */
	    case 0x4f4c: /* Xceed original location extra field */
	    case 0x5356: /* AOS/VS (ACL) */
	    case 0x5455: /* extended timestamp */
	    case 0x554e: /* Xceed unicode extra field */
	    case 0x5855: /* Info-ZIP Unix (original, also OS/2, NT, etc) */
	    case 0x6542: /* BeOS/BeBox */
	    case 0x756e: /* ASi Unix */
	    case 0x7855: /* Info-ZIP Unix (new) */
	    case 0xfd4a: /* SMS/QDOS */
		break;
	    }
	}
    }
    return item;
}
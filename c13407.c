int generate_password(int length, unsigned char *password)
{
    const char pwchars[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd',
        'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
        's', 't', 'u', 'v', 'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F',
        'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z', '%', '$'
    };

    FILE *randfp;
    unsigned char pwtemp[MAX_PASSWD_BUF];
    unsigned char *p;
    int i, n;
    int passlen;
    
    if ((length <= 0) || (length > MAX_PASSWD_LEN))
    {
        fprintf(stderr, "Invalid password length specified.\n");
        return -1;
    }

    /* Open the device to read random octets */
    if ((randfp = fopen("/dev/urandom", "r")) == NULL)
    {
        perror("Error open /dev/urandom:");
        return  -1;
    }

    /* Read random octets */
    if ((n = fread((char*)pwtemp, 1, length, randfp)) != length)
    {
        fprintf(stderr, "Error: Couldn't read from /dev/urandom\n");
        fclose(randfp);
        return  -1;
    }
    fclose(randfp);

    /* Now ensure each octet is uses the defined character set */
    for(i = 0, p = pwtemp; i < length; i++, p++)
    {
        *p = pwchars[((int)(*p)) % 64];
    }

    /* Convert the password to UTF-16LE */
    passlen = passwd_to_utf16(  pwtemp,
                                length,
                                MAX_PASSWD_LEN,
                                password);

    return passlen;
}
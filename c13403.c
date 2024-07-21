int read_password(unsigned char* buffer, encryptmode_t mode)
{
#ifndef WIN32
#define PASS_EOF EOF
    struct termios t;                   /* Used to set ECHO attribute */
    int echo_enabled;                   /* Was echo enabled? */
    int tty;                            /* File descriptor for tty */
    FILE* ftty;                         /* File for tty */
    unsigned char pwd[MAX_PASSWD_BUF];
    unsigned char pwd_confirm[MAX_PASSWD_BUF];
    unsigned char* p;                   /* Password buffer pointer */
#else
#define PASS_EOF L'\x003'
    FILE* ftty = stderr;                /* File for tty */
    wchar_t* pwd = (wchar_t *)buffer;
    wchar_t pwd_confirm[MAX_PASSWD_LEN+1];
    wchar_t* p;                         /* Password buffer pointer */
#endif
                                        /* Used for password confirmation */
    int c;                              /* Character read from input */
    int chars_read;                     /* Chars read from input */
    int i;                              /* Loop counter */
    int match;                          /* Do the two passwords match? */

#ifndef WIN32
    /* Open the tty */
    ftty = fopen("/dev/tty", "r+");
    if (ftty == NULL)
    {
        return AESCRYPT_READPWD_FOPEN;
    }
    tty = fileno(ftty);
    if (tty < 0)
    {
        return AESCRYPT_READPWD_FILENO;
    }
 
    /* Get the tty attrs */
    if (tcgetattr(tty, &t) < 0)
    {
        fclose(ftty);
        return AESCRYPT_READPWD_TCGETATTR;
    }
#endif

    /*
     * Round 1 - Read the password into buffer
     * (If encoding) Round 2 - read password 2 for confirmation
     */
    for (i = 0; (i == 0) || (i == 1 && mode == ENC); i++)
    {
        /* Choose the buffer where to put the password */
        if (!i)
        {
            p = pwd;
        }
        else
        {
            p = pwd_confirm;
        }

        /* Prompt for password */
        if (i)
        {
            fprintf(ftty, "Re-");
        }
        fprintf(ftty, "Enter password: ");
        fflush(ftty);

#ifndef WIN32
        /* Disable echo if necessary */
        if (t.c_lflag & ECHO)
        {
            t.c_lflag &= ~ECHO;
            if (tcsetattr(tty, TCSANOW, &t) < 0)
            {
                /* For security reasons, erase the password */
                memset_secure(pwd, 0, MAX_PASSWD_BUF);
                memset_secure(pwd_confirm, 0, MAX_PASSWD_BUF);
                fclose(ftty);
                return AESCRYPT_READPWD_TCSETATTR;
            }
            echo_enabled = 1;
        }
        else
        {
            echo_enabled = 0;
        }
#endif

        /* Read from input and fill buffer till MAX_PASSWD_LEN chars are read */
        chars_read = 0;
#ifdef WIN32
        while (((c = _getwch()) != L'\r') && (c != PASS_EOF))
#else
        while (((c = fgetc(ftty)) != '\n') && (c != PASS_EOF))
#endif
        {
            /* fill buffer till MAX_PASSWD_LEN */
            if (chars_read <= MAX_PASSWD_LEN)
            {
#ifdef WIN32
                p[chars_read] = (wchar_t) c;
#else
                p[chars_read] = (char) c;
#endif
            }
            chars_read++;
        }

        if (chars_read <= MAX_PASSWD_LEN)
        {
            p[chars_read] = '\0';
        }

        fprintf(ftty, "\n");

#ifndef WIN32
        /* Enable echo if disabled above */
        if (echo_enabled)
        {
            t.c_lflag |= ECHO;
            if (tcsetattr(tty, TCSANOW, &t) < 0)
            {
                /* For security reasons, erase the password */
                memset_secure(pwd, 0, MAX_PASSWD_BUF);
                memset_secure(pwd_confirm, 0, MAX_PASSWD_BUF);
                fclose(ftty);
                return AESCRYPT_READPWD_TCSETATTR;
            }
        }
#endif

        /* check for EOF error */
        if (c == PASS_EOF)
        {
            /* For security reasons, erase the password */
            memset_secure(pwd, 0, MAX_PASSWD_BUF);
            memset_secure(pwd_confirm, 0, MAX_PASSWD_BUF);
            if (ftty != stderr)
                fclose(ftty);
            return AESCRYPT_READPWD_FGETC;
        }

        /*
         * Check chars_read.  The password must be maximum MAX_PASSWD_LEN
         * chars.  If too long an error is returned
         */
        if (chars_read > MAX_PASSWD_LEN)
        {
            /* For security reasons, erase the password */
            memset_secure(pwd, 0, MAX_PASSWD_BUF);
            memset_secure(pwd_confirm, 0, MAX_PASSWD_BUF);
            if (ftty != stderr)
                fclose(ftty);
            return AESCRYPT_READPWD_TOOLONG;
        }
    }

    /* Close the tty */
    if (ftty != stderr)
        fclose(ftty);

    /* Password must be compared only when encrypting */
    if (mode == ENC)
    {
        /* Check if passwords match */
        match = strcmp((char*)pwd, (char*)pwd_confirm);
        memset_secure(pwd_confirm, 0, MAX_PASSWD_BUF);

        if (match != 0)
        {
            /* For security reasons, erase the password */
            memset_secure(pwd, 0, MAX_PASSWD_BUF);
            return AESCRYPT_READPWD_NOMATCH;
        }
    }

#ifdef WIN32
    chars_read *= 2;
#else
    chars_read = passwd_to_utf16(
       pwd,
       chars_read,
       MAX_PASSWD_LEN,
       buffer);

    if (chars_read < 0) {
        memset_secure(pwd_confirm, 0, MAX_PASSWD_BUF);
        memset_secure(pwd, 0, MAX_PASSWD_BUF);
        return AESCRYPT_READPWD_ICONV;
    }
#endif

    return chars_read;
}
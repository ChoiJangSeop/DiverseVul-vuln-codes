static int get_fsuid(void)
{
    int real, euid, saved;
    /* if we fail to parse the uid, then make it root only readable to be safe */
    int fs_uid = 0;

    char *line = proc_pid_status; /* never NULL */
    for (;;)
    {
        if (strncmp(line, "Uid", 3) == 0)
        {
            int n = sscanf(line, "Uid:\t%d\t%d\t%d\t%d\n", &real, &euid, &saved, &fs_uid);
            if (n != 4)
            {
                perror_msg_and_die("Can't parse Uid: line");
            }
            break;
        }
        line = strchr(line, '\n');
        if (!line)
            break;
        line++;
    }

    return fs_uid;
}
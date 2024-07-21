int main(int argc, char *argv[])
{
    // disable ptrace
#if HAVE_PR_SET_DUMPABLE
    prctl(PR_SET_DUMPABLE, 0);
#endif
#if HAVE_PROC_TRACE_CTL
    int mode = PROC_TRACE_CTL_DISABLE;
    procctl(P_PID, getpid(), PROC_TRACE_CTL, &mode);
#endif

    QApplication app(argc, argv);

    // FIXME: this can be considered a poor man's solution, as it's not
    // directly obvious to a gui user. :)
    // anyway, i vote against removing it even when we have a proper gui
    // implementation.  -- ossi
    QByteArray duser = qgetenv("ADMIN_ACCOUNT");
    if (duser.isEmpty())
        duser = "root";

    KLocalizedString::setApplicationDomain("kdesu");

    KAboutData aboutData("kdesu", i18n("KDE su"),
            Version, i18n("Runs a program with elevated privileges."),
            KAboutLicense::Artistic,
            i18n("Copyright (c) 1998-2000 Geert Jansen, Pietro Iglio"));
    aboutData.addAuthor(i18n("Geert Jansen"), i18n("Maintainer"),
            "jansen@kde.org", "http://www.stack.nl/~geertj/");
    aboutData.addAuthor(i18n("Pietro Iglio"), i18n("Original author"),
            "iglio@fub.it");
    app.setWindowIcon(QIcon::fromTheme("dialog-password"));

    KAboutData::setApplicationData(aboutData);

    // NOTE: if you change the position of the -u switch, be sure to adjust it
    // at the beginning of main()
    QCommandLineParser parser;
    parser.addPositionalArgument("command", i18n("Specifies the command to run as separate arguments"));
    parser.addOption(QCommandLineOption("c", i18n("Specifies the command to run as one string"), "command"));
    parser.addOption(QCommandLineOption("f", i18n("Run command under target uid if <file> is not writable"), "file"));
    parser.addOption(QCommandLineOption("u", i18n("Specifies the target uid"), "user", duser));
    parser.addOption(QCommandLineOption("n", i18n("Do not keep password")));
    parser.addOption(QCommandLineOption("s", i18n("Stop the daemon (forgets all passwords)")));
    parser.addOption(QCommandLineOption("t", i18n("Enable terminal output (no password keeping)")));
    parser.addOption(QCommandLineOption("p", i18n("Set priority value: 0 <= prio <= 100, 0 is lowest"), "prio", "50"));
    parser.addOption(QCommandLineOption("r", i18n("Use realtime scheduling")));
    parser.addOption(QCommandLineOption("noignorebutton", i18n("Do not display ignore button")));
    parser.addOption(QCommandLineOption("i", i18n("Specify icon to use in the password dialog"), "icon name"));
    parser.addOption(QCommandLineOption("d", i18n("Do not show the command to be run in the dialog")));
#ifdef HAVE_X11
    /* KDialog originally used --embed for attaching the dialog box.  However this is misleading and so we changed to --attach.
     * For consistancy, we silently map --embed to --attach */
    parser.addOption(QCommandLineOption("attach", i18nc("Transient means that the kdesu app will be attached to the app specified by the winid so that it is like a dialog box rather than some separate program", "Makes the dialog transient for an X app specified by winid"), "winid"));
    parser.addOption(QCommandLineOption("embed", i18n("Embed into a window"), "winid"));
#endif
    parser.addHelpOption();
    parser.addVersionOption();


    //KApplication::disableAutoDcopRegistration();
    // kdesu doesn't process SM events, so don't even connect to ksmserver
    QByteArray session_manager = qgetenv( "SESSION_MANAGER" );
    if (!session_manager.isEmpty())
        unsetenv( "SESSION_MANAGER" );
    // but propagate it to the started app
    if (!session_manager.isEmpty())
        setenv( "SESSION_MANAGER", session_manager.data(), 1 );

    {
#ifdef HAVE_X11
        KStartupInfoId id;
        id.initId();
        id.setupStartupEnv(); // make DESKTOP_STARTUP_ID env. var. available again
#endif
    }

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    int result = startApp(parser);

    if (result == 127)
    {
        KMessageBox::sorry(0, i18n("Cannot execute command '%1'.", QString::fromLocal8Bit(command)));
    }

    return result;
}
set_message(
    message_t *message,
    int        want_quoted)
{
    char *msg = NULL;
    char *hint = NULL;
    GString *result;

    init_errcode();

    if (message == NULL)
	return;

    if (message->code == 123) {
	msg  = "%{errstr}";
    } else if (message->code == 2800000) {
	msg  = "Usage: amcheck [--version] [-am] [-w] [-sclt] [-M <address>] [--client-verbose] [--exact_match] [-o configoption]* <conf> [host [disk]* ]*";
    } else if (message->code == 2800001) {
	msg  = "amcheck-%{version}";
    } else if (message->code == 2800002) {
	msg  = "Multiple -M options";
    } else if (message->code == 2800003) {
	msg  = "Invalid characters in mail address";
    } else if (message->code == 2800004) {
	msg  = "You can't use -a because a mailer is not defined";
    } else if (message->code == 2800005) {
	msg  = "You can't use -m because a mailer is not defined";
    } else if (message->code == 2800006) {
	msg  = "No mail address configured in amanda.conf";
	hint = "To receive dump results by email configure the "
                 "\"mailto\" parameter in amanda.conf";
    } else if (message->code == 2800007) {
	msg  = "To receive dump results by email configure the "
                 "\"mailto\" parameter in amanda.conf";
    } else if (message->code == 2800008) {
	msg  = "When using -a option please specify -Maddress also";
    } else if (message->code == 2800009) {
	msg  = "Use -Maddress instead of -m";
    } else if (message->code == 2800010) {
	msg  = "Mail address '%{mailto}' in amanda.conf has invalid characters";
    } else if (message->code == 2800011) {
	msg  = "No email will be sent";
    } else if (message->code == 2800012) {
	msg  = "No mail address configured in amanda.conf";
    } else if (message->code == 2800013) {
	msg  = "When using -a option please specify -Maddress also";
    } else if (message->code == 2800014) {
	msg  = "Use -Maddress instead of -m";
    } else if (message->code == 2800015) {
	msg  = "%{errstr}";
    } else if (message->code == 2800016) {
	msg  = "(brought to you by Amanda %{version})";
    } else if (message->code == 2800017) {
	msg  = "Invalid mailto address '%{mailto}'";
    } else if (message->code == 2800018) {
	msg  = "tapelist '%{tapelist}': should be a regular file";
    } else if (message->code == 2800019) {
	msg  = "can't access tapelist '%{tapelist}': %{errnostr}";
    } else if (message->code == 2800020) {
	msg  = "tapelist '%{tapelist}': not writable: %{errnostr}";
    } else if (message->code == 2800021) {
	msg  = "parent: reaped bogus pid %{pid}";
    } else if (message->code == 2800022) {
	msg  = "program %{program}: does not exist";
    } else if (message->code == 2800023) {
	msg  = "program %{program}: not a file";
    } else if (message->code == 2800024) {
	msg  = "program %{program}: not executable";
    } else if (message->code == 2800025) {
	msg  = "program %{program}: not setuid-root";
    } else if (message->code == 2800026) {
	msg  = "amcheck-device terminated with signal %{signal}";
    } else if (message->code == 2800027) {
	msg  = "Amanda Tape Server Host Check";
    } else if (message->code == 2800028) {
	msg  = "-----------------------------";
    } else if (message->code == 2800029) {
	msg  = "storage '%{storage}': cannot read label template (lbl-templ) file %{filename}: %{errnostr}";
	hint = "check permissions";
    } else if (message->code == 2800030) {
	msg  = "storage '%{storage}': lbl-templ set but no LPR command defined";
	hint = "you should reconfigure amanda and make sure it finds a lpr or lp command";
    } else if (message->code == 2800031) {
	msg  = "storage '%{storage}': flush-threshold-dumped (%{flush_threshold_dumped}) must be less than or equal to flush-threshold-scheduled (%{flush_threshold_scheduled})";
    } else if (message->code == 2800032) {
	msg  = "storage '%{storage}': taperflush (%{taperflush}) must be less than or equal to flush-threshold-scheduled (%{flush_threshold_scheduled})";
    } else if (message->code == 2800033) {
	msg  = "WARNING: storage '%{storage}': autoflush must be set to 'yes' or 'all' if taperflush (%{taperflush}) is greater that 0";
    } else if (message->code == 2800034) {
	msg  = "storage '%{storage}': no tapetype specified; you must give a value for the 'tapetype' parameter or the storage";
    } else if (message->code == 2800035) {
	msg  = "storage '%{storage}': runtapes is larger or equal to policy '%{policy}' retention-tapes";
    } else if (message->code == 2800036) {
	msg  = "system has %{size:kb_avail} memory, but device-output-buffer-size needs {size:kb_needed}";
    } else if (message->code == 2800037) {
	msg  = "Cannot resolve `localhost': %{gai_strerror}";
    } else if (message->code == 2800038) {
	msg  = "directory '%{dir}' containing Amanda tools is not accessible: %{errnostr}";
	hint = "check permissions";
    } else if (message->code == 2800039) {
	msg = "Check permissions";
    } else if (message->code == 2800040) {
	msg  = "directory '%{dir}' containing Amanda tools is not accessible: %{errnostr}";
	hint = "check permissions";
    } else if (message->code == 2800041) {
	msg  = "Check permissions";
    } else if (message->code == 2800042) {
	msg  = "WARNING: '%{program}' is not executable: %{errnostr}, server-compression and indexing will not work";
	hint = "check permissions";
    } else if (message->code == 2800043) {
	msg  = "Check permissions";
    } else if (message->code == 2800044) {
	msg  = "tapelist dir '%{tape_dir}': not writable: %{errnostr}";
	hint = "check permissions";
    } else if (message->code == 2800045) {
	msg  = "tapelist '%{tapefile}' (%{errnostr}), you must create an empty file";
    } else if (message->code == 2800046) {
	msg  = "tapelist file do not exists";
	hint = "it will be created on the next run";
    } else if (message->code == 2800047) {
	msg  = "tapelist '%{tapefile}': parse error";
    } else if (message->code == 2800048) {
	msg  = "hold file '%{holdfile}' exists. Amdump will sleep as long as this file exists";
	hint = "You might want to delete the existing hold file";
    } else if (message->code == 2800049) {
	msg  = "Amdump will sleep as long as this file exists";
    } else if (message->code == 2800050) {
	msg  = "You might want to delete the existing hold file";
    } else if (message->code == 2800051) {
	msg  = "WARNING:Parameter \"tapedev\", \"tpchanger\" or storage not specified in amanda.conf";
    } else if (message->code == 2800052) {
	msg  = "part-cache-type specified, but no part-size";
    } else if (message->code == 2800053) {
	msg  = "part-cache-dir specified, but no part-size";
    } else if (message->code == 2800054) {
	msg  = "part-cache-max-size specified, but no part-size";
    } else if (message->code == 2800055) {
	msg  = "part-cache-type is DISK, but no part-cache-dir specified";
    } else if (message->code == 2800056) {
	msg  = "part-cache-dir '%{part-cache-dir}': %{errnostr}";
    } else if (message->code == 2800057) {
	msg  = "part-cache-dir has %{size:kb_avail} available, but needs %{size:kb_needed}";
    } else if (message->code == 2800058) {
	msg  = "system has %{size:kb_avail} memory, but part cache needs %{size:kb_needed}";
    } else if (message->code == 2800059) {
	msg  = "part-cache-dir specified, but part-cache-type is not DISK";
    } else if (message->code == 2800060) {
	msg  = "part_size is zero, but part-cache-type is not 'none'";
    } else if (message->code == 2800061) {
	msg  = "part-cache-max-size is specified but no part cache is in use";
    } else if (message->code == 2800062) {
	msg  = "WARNING: part-cache-max-size is greater than part-size";
    } else if (message->code == 2800063) {
	msg  = "WARNING: part-size of %{size:part_size} < 0.1%% of tape length";
    } else if (message->code == 2800064) {
	msg  = "This may create > 1000 parts, severely degrading backup/restore performance."
        " See http://wiki.zmanda.com/index.php/Splitsize_too_small for more information.";
    } else if (message->code == 2800065) {
	msg  = "part-cache-max-size of %{size:part_size_max_size} < 0.1%% of tape length";
    } else if (message->code == 2800066) {
	msg  = "holding dir '%{holding_dir}' (%{errnostr})";
	hint = "you must create a directory";
    } else if (message->code == 2800067) {
	msg  = "holding disk '%{holding_dir}': not writable: %{errnostr}";
	hint = "check permissions";
    } else if (message->code == 2800068) {
	msg  = "Check permissions";
    } else if (message->code == 2800069) {
	msg  = "holding disk '%{holding_dir}': not searcheable: %{errnostr}";
	hint = "check permissions of ancestors";
    } else if (message->code == 2800070) {
	msg  = "Check permissions of ancestors of";
    } else if (message->code == 2800071) {
	msg  = "WARNING: holding disk '%{holding_dir}': no space available (%{size:size} requested)";
    } else if (message->code == 2800072) {
	msg  = "WARNING: holding disk '%{holding_dir}': only %{size:avail} available (%{size:requested} requested)";
    } else if (message->code == 2800073) {
	msg = "Holding disk '%{holding_dir}': %{size:avail} disk space available, using %{size:requested} as requested";
    } else if (message->code == 2800074) {
	msg  = "holding disk '%{holding_dir}': only %{size:avail} free, using nothing";
    } else if (message->code == 2800075) {
	msg = "Not enough free space specified in amanda.conf";
    } else if (message->code == 2800076) {
	msg  = "Holding disk '%{holding_dir}': %{size:avail} disk space available, using %{size:using}";
    } else if (message->code == 2800077) {
	msg  = "logdir '%{logdir}' (%{errnostr})";
	hint = "you must create directory";
    } else if (message->code == 2800078) {
	msg  = "log dir '%{logdir}' (%{errnostr}): not writable";
    } else if (message->code == 2800079) {
	msg  = "oldlog directory '%{olddir}' is not a directory";
	hint = "remove the entry and create a new directory";
    } else if (message->code == 2800080) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800081) {
	msg  = "oldlog dir '%{oldlogdir}' (%{errnostr}): not writable";
	hint = "check permissions";
    } else if (message->code == 2800082) {
	msg  = "Check permissions";
    } else if (message->code == 2800083) {
	msg  = "oldlog directory '%{oldlogdir}' (%{errnostr}) is not a directory";
	hint = "remove the entry and create a new directory";
    } else if (message->code == 2800084) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800085) {
	msg  = "skipping tape test because amdump or amflush seem to be running";
	hint = "if they are not, you must run amcleanup";
    } else if (message->code == 2800086) {
	msg  = "if they are not, you must run amcleanup";
    } else if (message->code == 2800087) {
	msg  = "amdump or amflush seem to be running";
	hint = "if they are not, you must run amcleanup";
    } else if (message->code == 2800088) {
	msg  = "if they are not, you must run amcleanup";
    } else if (message->code == 2800089) {
	msg  = "skipping tape checks";
    } else if (message->code == 2800090) {
	msg  = "tapecycle (%{tapecycle}) <= runspercycle (%{runspercycle}";
    } else if (message->code == 2800091) {
	msg  = "tapecycle (%{tapecycle}) <= runtapes (%{runtapes})";
    } else if (message->code == 2800092) {
	msg  = "conf info dir '%{infodir}' does not exist";
	hint = "it will be created on the next run";
    } else if (message->code == 2800093) {
	msg  = "it will be created on the next run.";
    } else if (message->code == 2800094) {
	msg  = "conf info dir '%{infodir}' (%{errnostr})";
    } else if (message->code == 2800095) {
	msg  = "info dir '%{infodir}': not a directory";
	hint = "remove the entry and create a new directory";
    } else if (message->code == 2800096) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800097) {
	msg  = "info dir '{infodir}' (%{errnostr}): not writable";
	hint = "check permissions";
    } else if (message->code == 2800098) {
	msg  = "Check permissions";
    } else if (message->code == 2800099) {
	msg  = "Can't copy infofile: %{errmsg}";
    } else if (message->code == 2800100) {
	msg  = "host info dir '%{hostinfodir}' does not exist";
	hint = "It will be created on the next run";
    } else if (message->code == 2800101) {
	msg  = "it will be created on the next run";
    } else if (message->code == 2800102) {
	msg  = "host info dir '%{hostinfodir}' (%{errnostr})";
    } else if (message->code == 2800103) {
	msg  = "info dir '%{hostinfodir}': not a directory";
	hint = "Remove the entry and create a new directory";
    } else if (message->code == 2800104) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800105) {
	msg  = "info dir '%{hostinfodir}': not writable";
	hint = "Check permissions";
    } else if (message->code == 2800106) {
	msg  = "Check permissions";
    } else if (message->code == 2800107) {
	msg  = "info dir '%{diskdir}' does not exist";
	hint = "it will be created on the next run";
    } else if (message->code == 2800108) {
	msg  = "it will be created on the next run.";
    } else if (message->code == 2800109) {
	msg  = "info dir '%{diskdir}' (%{errnostr})";
    } else if (message->code == 2800110) {
	msg  = "info dir '%{diskdir}': not a directory";
	hint = "Remove the entry and create a new directory";
    } else if (message->code == 2800111) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800112) {
	msg  = "info dir '%{diskdir}': not writable";
	hint = "Check permissions";
    } else if (message->code == 2800113) {
	msg  = "Check permissions";
    } else if (message->code == 2800114) {
	msg  = "info file '%{infofile}' does not exist";
	hint = "it will be created on the next run";
    } else if (message->code == 2800115) {
	msg  = "it will be created on the next run";
    } else if (message->code == 2800116) {
	msg  = "info dir '%{diskdir}' (%{errnostr})";
    } else if (message->code == 2800117) {
	msg  = "info file '%{infofile}': not a file";
	hint = "remove the entry and create a new file";
    } else if (message->code == 2800118) {
	msg  = "Remove the entry and create a new file";
    } else if (message->code == 2800119) {
	msg  = "info file '%{infofile}': not readable";
	hint = "Check permissions";
    } else if (message->code == 2800120) {
	msg  = "index dir '%{indexdir}' does not exist";
	hint = "it will be created on the next run";
    } else if (message->code == 2800121) {
	msg  = "it will be created on the next run.";
    } else if (message->code == 2800122) {
	msg  = "index dir '%{indexdir}' (%{errnostr})";
    } else if (message->code == 2800123) {
	msg  = "index dir '%{indexdir}': not a directory";
	hint = "remove the entry and create a new directory";
    } else if (message->code == 2800124) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800125) {
	msg  = "index dir '%{indexdir}': not writable";
    } else if (message->code == 2800126) {
	msg  = "index dir '%{hostindexdir}' does not exist";
	hint = "it will be created on the next run";
    } else if (message->code == 2800127) {
	msg  = "it will be created on the next run.";
    } else if (message->code == 2800128) {
	msg  = "index dir '%{hostindexdir}' (%{errnostr})";
    } else if (message->code == 2800129) {
	msg  = "index dir '%{hostindexdir}': not a directory";
	hint = "remove the entry and create a new directory";
    } else if (message->code == 2800130) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800131) {
	msg  = "index dir '%{hostindexdir}': not writable";
	hint = "check permissions";
    } else if (message->code == 2800132) {
	msg  = "index dir '%{diskindexdir}' does not exist";
	hint = "it will be created on the next run";
    } else if (message->code == 2800133) {
	msg  = "it will be created on the next run.";
    } else if (message->code == 2800134) {
	msg  = "index dir '%{diskindexdir}' (%{errnostr})";
	hint = "check permissions";
    } else if (message->code == 2800135) {
	msg  = "index dir '%{diskindexdir}': not a directory";
	hint = "remove the entry and create a new directory";
    } else if (message->code == 2800136) {
	msg  = "Remove the entry and create a new directory";
    } else if (message->code == 2800137) {
	msg  = "index dir '%{diskindexdir}': is not writable";
	hint = "check permissions";
    } else if (message->code == 2800138) {
	msg  = "server encryption program not specified";
	hint = "Specify \"server-custom-encrypt\" in the dumptype";
    } else if (message->code == 2800139) {
	msg  = "Specify \"server-custom-encrypt\" in the dumptype";
    } else if (message->code == 2800140) {
	msg  = "'%{program}' is not executable, server encryption will not work";
	hint = "check file type";
    } else if (message->code == 2800141) {
	msg  = "Check file type";
    } else if (message->code == 2800142) {
	msg  = "server custom compression program not specified";
	hint = "Specify \"server-custom-compress\" in the dumptype";
    } else if (message->code == 2800143) {
	msg  = "Specify \"server-custom-compress\" in the dumptype";
    } else if (message->code == 2800144) {
	msg  = "'%{program}' is not executable, server custom compression will not work";
	hint = "check file type";
    } else if (message->code == 2800145) {
	msg  = "Check file type";
    } else if (message->code == 2800146) {
	msg  = "%{hostname} %{diskname}: tape-splitsize > tape size";
    } else if (message->code == 2800147) {
	msg  = "%{hostname} %{diskname}: fallback-splitsize > total available memory";
    } else if (message->code == 2800148) {
	msg  = "%{hostname} %{diskname}: fallback-splitsize > tape size";
    } else if (message->code == 2800149) {
	msg  = "%{hostname} %{diskname}: tape-splitsize of %{size:tape_splitsize} < 0.1%% of tape length";
    } else if (message->code == 2800151) {
	msg  = "%{hostname} %{diskname}: fallback-splitsize of %{size:fallback_splitsize} < 0.1%% of tape length";
    } else if (message->code == 2800153) {
	msg  = "%{hostname} %{diskname}: Can't compress directtcp data-path";
    } else if (message->code == 2800154) {
	msg  = "%{hostname} %{diskname}: Can't encrypt directtcp data-path";
    } else if (message->code == 2800155) {
	msg  = "%{hostname} %{diskname}: Holding disk can't be use for directtcp data-path";
    } else if (message->code == 2800156) {
	msg  = "%{hostname} %{diskname}: data-path is DIRECTTCP but device do not support it";
    } else if (message->code == 2800157) {
	msg  = "%{hostname} %{diskname}: data-path is AMANDA but device do not support it";
    } else if (message->code == 2800158) {
	msg  = "%{hostname} %{diskname}: Can't run pre-host-backup script on client";
    } else if (message->code == 2800159) {
	msg  = "%{hostname} %{diskname}: Can't run post-host-backup script on client";
    } else if (message->code == 2800160) {
	msg  = "Server check took %{seconds} seconds";
    } else if (message->code == 2800161) {
	msg  = "Client %{hostname} does not support selfcheck REQ packet";
	hint = "Client might be of a very old version";
    } else if (message->code == 2800162) {
	msg  = "Client might be of a very old version";
    } else if (message->code == 2800163) {
	msg  = "Client %{hostname} does not support selfcheck REP packet";
	hint = "Client might be of a very old version";
    } else if (message->code == 2800164) {
	msg  = "Client might be of a very old version";
    } else if (message->code == 2800165) {
	msg  = "Client %{hostname} does not support sendsize REQ packet";
	hint = "Client might be of a very old version";
    } else if (message->code == 2800166) {
	msg  = "Client might be of a very old version";
    } else if (message->code == 2800167) {
	msg  = "Client %{hostname} does not support sendsize REP packet";
	hint = "Client might be of a very old version";
    } else if (message->code == 2800168) {
	msg  = "Client might be of a very old version";
    } else if (message->code == 2800169) {
	msg  = "Client %{hostname} does not support sendbackup REQ packet";
	hint = "Client might be of a very old version";
    } else if (message->code == 2800170) {
	msg  = "Client might be of a very old version";
    } else if (message->code == 2800171) {
	msg  = "Client %{hostname} does not support sendbackup REP packet";
	hint = "Client might be of a very old version";
    } else if (message->code == 2800172) {
	msg  = "Client might be of a very old version";
    } else if (message->code == 2800173) {
	msg  = "%{hostname}:%{diskname} %{errstr}";
    } else if (message->code == 2800174) {
	msg  = "%{hostname}:%{diskname} (%{device}) host does not support quoted text";
	hint = "You must upgrade amanda on the client to "
                                    "specify a quoted text/device in the disklist, "
                                    "or don't use quoted text for the device";
    } else if (message->code == 2800175) {
	msg  = "You must upgrade amanda on the client to "
                                    "specify a quoted text/device in the disklist, "
                                    "or don't use quoted text for the device";
    } else if (message->code == 2800176) {
	msg  = "%{hostname}:%{diskname} (%{device}): selfcheck does not support device";
	hint = "You must upgrade amanda on the client to "
                                    "specify a diskdevice in the disklist "
                                    "or don't specify a diskdevice in the disklist";
    } else if (message->code == 2800177) {
	msg  = "You must upgrade amanda on the client to "
                                    "specify a diskdevice in the disklist "
                                    "or don't specify a diskdevice in the disklist";
    } else if (message->code == 2800178) {
	msg  = "%{hostname}:%{diskname} (%{device}): sendsize does not support device";
	hint = "You must upgrade amanda on the client to "
                                    "specify a diskdevice in the disklist"
                                    "or don't specify a diskdevice in the disklist";
    } else if (message->code == 2800179) {
	msg  = "You must upgrade amanda on the client to "
                                    "specify a diskdevice in the disklist"
                                    "or don't specify a diskdevice in the disklist";
    } else if (message->code == 2800180) {
	msg  = "%{hostname}:%{diskname} (%{device}): sendbackup does not support device";
	hint = "You must upgrade amanda on the client to "
                                    "specify a diskdevice in the disklist"
                                    "or don't specify a diskdevice in the disklist";
    } else if (message->code == 2800181) {
	msg  = "You must upgrade amanda on the client to "
                                    "specify a diskdevice in the disklist"
                                    "or don't specify a diskdevice in the disklist";
    } else if (message->code == 2800182) {
	msg  = "Client %{hostname} does not support %{data-path} data-path";
    } else if (message->code == 2800183) {
	msg  = "Client %{hostname} does not support directtcp data-path";
    } else if (message->code == 2800184) {
	msg  = "%{hostname}:%{diskname} does not support DUMP";
	hint = "You must upgrade amanda on the client to use DUMP "
                                    "or you can use another program";
    } else if (message->code == 2800185) {
	msg  = "You must upgrade amanda on the client to use DUMP "
                                    "or you can use another program";
    } else if (message->code == 2800186) {
	msg  = "%{hostname}:%{diskname} does not support GNUTAR";
	hint = "You must upgrade amanda on the client to use GNUTAR "
                                    "or you can use another program";
    } else if (message->code == 2800187) {
	msg  = "You must upgrade amanda on the client to use GNUTAR "
                                    "or you can use another program";
    } else if (message->code == 2800188) {
	msg  = "%{hostname}:%{diskname} does not support CALCSIZE for estimate, using CLIENT";
	hint = "You must upgrade amanda on the client to use "
                                    "CALCSIZE for estimate or don't use CALCSIZE for estimate";
    } else if (message->code == 2800189) {
	msg  = "You must upgrade amanda on the client to use "
                                    "CALCSIZE for estimate or don't use CALCSIZE for estimate";
    } else if (message->code == 2800180) {
	msg  = "Client %{hostname} does not support custom compression";
	hint = "You must upgrade amanda on the client to use custom compression\n"
	       "Otherwise you can use the default client compression program";
    } else if (message->code == 2800191) {
	msg  = "You must upgrade amanda on the client to use custom compression";
    } else if (message->code == 2800192) {
	msg  = "Otherwise you can use the default client compression program";
    } else if (message->code == 2800193) {
	msg  = "Client %{hostname} does not support data encryption";
	hint = "You must upgrade amanda on the client to use encryption program";
    } else if (message->code == 2800194) {
	msg  = "You must upgrade amanda on the client to use encryption program";
    } else if (message->code == 2800195) {
	msg  = "%{hostname}: Client encryption with server compression is not supported";
	hint = "See amanda.conf(5) for detail";
    } else if (message->code == 2800196) {
	msg  = "%{hostname}:%{diskname} does not support APPLICATION-API";
	hint = "Dumptype configuration is not GNUTAR or DUMP. It is case sensitive";
    } else if (message->code == 2800197) {
	msg  = "Dumptype configuration is not GNUTAR or DUMP. It is case sensitive";
    } else if (message->code == 2800198) {
	msg  = "application '%{application}' not found";
    } else if (message->code == 2800199) {
	msg  = "%{hostname}:%{diskname} does not support client-name in application";
    } else if (message->code == 2800200) {
	msg  = "%{hostname}:%{diskname} does not support SCRIPT-API";
    } else if (message->code == 2800201) {
	msg  = "WARNING: %{hostname}:%{diskname} does not support client-name in script";
    } else if (message->code == 2800202) {
	msg  = "Amanda Backup Client Hosts Check";
    } else if (message->code == 2800203) {
	msg  = "--------------------------------";
    } else if (message->code == 2800204) {
	int hostcount = atoi(message_get_argument(message, "hostcount"));
	int remote_errors = atoi(message_get_argument(message, "remote_errors"));
	char *a = plural("Client check: %{hostcount} host checked in %{seconds} seconds.",
                         "Client check: %{hostcount} hosts checked in %{seconds} seconds.",
                         hostcount);
	char *b = plural("  %{remote_errors} problem found.",
                         "  %{remote_errors} problems found.",
                         remote_errors);
	msg  = g_strdup_printf("%s%s", a, b);
    } else if (message->code == 2800206) {
	msg  = "%{hostname}: selfcheck request failed: %{errstr}";
    } else if (message->code == 2800207) {
	msg  = "%{hostname}: bad features value: '%{features}'";
	hint = "The amfeature in the reply packet is invalid";
    } else if (message->code == 2800208) {
	msg  = "The amfeature in the reply packet is invalid";
    } else if (message->code == 2800209) {
	msg  = "%{dle_hostname}";
    } else if (message->code == 2800210) {
	msg  = "%{ok_line}";
    } else if (message->code == 2800211) {
	msg  = "%{type}%{hostname}: %{errstr}";
    } else if (message->code == 2800212) {
	msg  = "%{hostname}: unknown response: %{errstr}";
    } else if (message->code == 2800213) {
	msg  = "Could not find security driver '%{auth}' for host '%{hostname}'. auth for this dle is invalid";
    } else if (message->code == 2800214) {
    } else if (message->code == 2800215) {
	msg  = "amanda.conf has dump user configured to '%{dumpuser}', but that user does not exist";
    } else if (message->code == 2800216) {
	msg  = "cannot get username for running user, uid %{uid} is not in your user database";
    } else if (message->code == 2800217) {
	msg  = "must be executed as the '%{expected_user}' user instead of the '%{running_user}' user";
	hint = "Change user to '%{expected_user}' or change dumpuser to '%{running_user}' in amanda.conf";
    } else if (message->code == 2800218) {
	msg  = "could not open temporary amcheck output file %{filename}: %{errnostr}";
	hint = "Check permissions";
    } else if (message->code == 2800219) {
	msg  = "could not open amcheck output file %{filename}: %{errnostr}";
	hint = "Check permissions";
    } else if (message->code == 2800220) {
	msg = "seek temp file: %{errnostr}";
    } else if (message->code == 2800221) {
	msg = "fseek main file: %{errnostr}";
    } else if (message->code == 2800222) {
	msg = "mailfd write: %{errnostr}";
    } else if (message->code == 2800223) {
	msg = "mailfd write: wrote %{size:write_size} instead of %{size:expected_size}";
    } else if (message->code == 2800224) {
	msg = "Can't fdopen: %{errnostr}";
    } else if (message->code == 2800225) {
	msg = "error running mailer %{mailer}: %{errmsg}";
    } else if (message->code == 2800226) {
	msg = "could not spawn a process for checking the server: %{errnostr}";
    } else if (message->code == 2800227) {
	msg = "nullfd: /dev/null: %{errnostr}";
    } else if (message->code == 2800228) {
	msg = "errors processing config file";
    } else if (message->code == 2800229) {
	msg = "Invalid mailto address '%{mailto}'";
    } else if (message->code == 2800230) {
	msg = "Can't open '%{filename}' for reading: %{errnostr}";
    } else if (message->code == 2800231) {
	msg = "Multiple DLE's for host '%{hostname}' use different auth methods";
	hint = "Please ensure that all DLE's for the host use the same auth method, including skipped ones";
    } else if (message->code == 2800232) {
	msg = "Multiple DLE's for host '%{hostname}' use different maxdumps values";
	hint = "Please ensure that all DLE's for the host use the same maxdumps value, including skipped ones";
    } else if (message->code == 2800233) {
	msg = "%{hostname} %{diskname}: The tag '%{tag}' match none of the storage dump_selection";
    } else if (message->code == 2800234) {
	msg = "%{hostname} %{diskname}: holdingdisk NEVER with tags matching more than one storage, will be dumped to only one storage";
    } else if (message->code == 2900000) {
	msg = "The Application '%{application}' failed: %{errmsg}";
    } else if (message->code == 2900001) {
	msg = "Can't execute application '%{application}'";
    } else if (message->code == 2900002) {
	msg = "The application '%{application}' does not support the '%{method}' method";
    } else if (message->code == 2900003) {
	msg = "%{service} only works with application";
    } else if (message->code == 2900004) {
	msg = "Missing OPTIONS line in %{service} request";
    } else if (message->code == 2900005) {
	msg = "Application '%{application}': can't create pipe";
    } else if (message->code == 2900006) {
	msg = "Can't dup2: %{errno} %{errnocode} %{errnostr}";
    } else if (message->code == 2900007) {
	msg = "%{service} require fe_req_xml";
    } else if (message->code == 2900008) {
	msg = "no valid %{service} request";
    } else if (message->code == 2900009) {
	msg = "no valid %{service} request";
    } else if (message->code == 2900010) {
	msg = "fork of '%{application} failed: %{errno} %{errnocode} %{errnostr}";
    } else if (message->code == 2900011) {
	msg = "Can't fdopen: %{errno} %{errnocode} %{errnostr}";
    } else if (message->code == 2900012) {
	msg = "%{application} failed: %{errmsg}";
    } else if (message->code == 2900013) {
	msg = "REQ XML error: %{errmsg}";
    } else if (message->code == 2900014) {
	msg = "One DLE required in XML REQ packet";
    } else if (message->code == 2900015) {
	msg = "Only one DLE allowed in XML REQ packet";
    } else if (message->code == 2900016) {
	msg = "Application '%{application}' (pid %{pid}) got signal %{signal}";
    } else if (message->code == 2900017) {
	msg = "Application '%{application}' (pid %{pid}) returned %{return_code}";
    } else if (message->code == 2900018) {
	msg = "%{name}: %{errmsg}";

    } else if (message->code == 3100005) {
	msg = "senddiscover result";
    } else if (message->code == 3100006) {
	msg = "no senddiscover result to list";

    } else if (message->code == 3600000) {
	msg = "version '%{version}";
    } else if (message->code == 3600001) {
	msg = "distro %{distro}";
    } else if (message->code == 3600002) {
	msg = "platform %{platform}";
    } else if (message->code == 3600003) {
	msg = "Multiple OPTIONS line in selfcheck input";
    } else if (message->code == 3600004) {
	msg = "OPTIONS features=%{features};hostname=%{hostname};";
    } else if (message->code == 3600005) {
	msg = "%{errstr}";
    } else if (message->code == 3600006) {
	msg = "Missing OPTIONS line in selfcheck input";
    } else if (message->code == 3600007) {
	msg = "FORMAT ERROR IN REQUEST PACKET %{err_extra}";
    } else if (message->code == 3600008) {
	msg = "FORMAT ERROR IN REQUEST PACKET";
    } else if (message->code == 3600009) {
	msg = "samba support only one exclude file";
    } else if (message->code == 3600010) {
	msg = "samba does not support exclude list";
    } else if (message->code == 3600011) {
	msg = "samba does not support include file";
    } else if (message->code == 3600012) {
	msg = "samba does not support include list";
    } else if (message->code == 3600013) {
	msg = "DUMP does not support exclude file";
    } else if (message->code == 3600014) {
	msg = "DUMP does not support exclude list";
    } else if (message->code == 3600015) {
	msg = "DUMP does not support include file";
    } else if (message->code == 3600016) {
	msg = "DUMP does not support include list";
    } else if (message->code == 3600017) {
	msg = "client configured for auth=%{auth} while server requested '%{auth-requested}'";
    } else if (message->code == 3600018) {
	msg = "The auth in ~/.ssh/authorized_keys should be \"--auth=ssh\", or use another auth for the DLE";
    } else if (message->code == 3600019) {
	msg = "The auth in the inetd/xinetd configuration must be the same as the DLE";
    } else if (message->code == 3600020) {
	msg = "%{device}: Can't use CALCSIZE for samba estimate, use CLIENT";
    } else if (message->code == 3600021) {
	msg = "%{device}: cannot parse for share/subdir disk entry";
    } else if (message->code == 3600022) {
	msg = "%{device}: subdirectory specified but samba is not v2 or better";
    } else if (message->code == 3600023) {
	msg = "%{device}: cannot find password";
    } else if (message->code == 3600024) {
	msg = "%{device}: password field not 'user%%pass'";
    } else if (message->code == 3600025) {
	msg = "%{device}: cannot make share name";
    } else if (message->code == 3600026) {
	msg = "%{device}: Cannot access /dev/null: %{errnostr}";
    } else if (message->code == 3600027) {
	msg = "%{device}: password write failed: %{errnostr}";
    } else if (message->code == 3600028) {
	msg = "%{device}: Can't fdopen ferr: %{errnostr}";
    } else if (message->code == 3600029) {
	msg = "%{device}: samba access error:";
    } else if (message->code == 3600030) {
	msg = "%{device}: This client is not configured for samba";
    } else if (message->code == 3600031) {
	msg = "%{device}: The DUMP program cannot handle samba shares, use the amsamba application";
    } else if (message->code == 3600032) {
	msg = "%{device}: Application '%{application}': %{errstr}";
    } else if (message->code == 3600033) {
	msg = "%{device}: Application '%{application}': can't run 'support' command";
    } else if (message->code == 3600034) {
	msg = "%{device}: Application '%{application}': doesn't support amanda data-path";
    } else if (message->code == 3600035) {
	msg = "%{device}: Application '%{application}': doesn't support directtcp data-path";
    } else if (message->code == 3600036) {
	msg = "%{device}: Application '%{application}': doesn't support calcsize estimate";
    } else if (message->code == 3600037) {
	msg = "%{device}: Application '%{application}': doesn't support include-file";
    } else if (message->code == 3600038) {
	msg = "%{device}: Application '%{application}': doesn't support include-list";
    } else if (message->code == 3600039) {
	msg = "%{device}: Application '%{application}': doesn't support optional include";
    } else if (message->code == 3600040) {
	msg = "%{device}: Application '%{application}': doesn't support exclude-file";
    } else if (message->code == 3600041) {
	msg = "%{device}: Application '%{application}': doesn't support exclude-list";
    } else if (message->code == 3600042) {
	msg = "%{device}: Application '%{application}': doesn't support optional exclude";
    } else if (message->code == 3600043) {
	msg = "%{device}: Application '%{application}': can't create pipe: %{errnostr}";
    } else if (message->code == 3600044) {
	msg = "%{device}: Application '%{application}': fork failed: %{errnostr}";
    } else if (message->code == 3600045) {
	msg = "%{device}: Can't execute '%{cmd}': %{errnostr}";
    } else if (message->code == 3600046) {
	msg = "%{device}: Can't fdopen app_stderr: %{errnostr}";
    } else if (message->code == 3600047) {
	msg = "%{device}: Application '%{application}': %{errstr}";
    } else if (message->code == 3600048) {
	msg = "%{device}: waitpid failed: %{errnostr}";
    } else if (message->code == 3600049) {
	msg = "%{device}: Application '%{application}': exited with signal %{signal}";
    } else if (message->code == 3600050) {
	msg = "%{device}: Application '%{application}': exited with status %{exit_status}";
    } else if (message->code == 3600051) {
	msg = "%{hostname}: Could not %{type} %{disk} (%{device}): %{errnostr}";
    } else if (message->code == 3600052) {
	msg = "%{disk}";
    } else if (message->code == 3600053) {
	msg = "%{amdevice}";
    } else if (message->code == 3600054) {
	msg = "%{device}";
    } else if (message->code == 3600055) {
	msg = "%{device}: Can't fdopen app_stdout: %{errnostr}";
    } else if (message->code == 3600056) {
	msg = "%{ok_line}";
    } else if (message->code == 3600057) {
	msg = "%{error_line}";
    } else if (message->code == 3600058) {
	msg = "%{errstr}";
    } else if (message->code == 3600059) {
	msg = "%{filename} is not a file";
    } else if (message->code == 3600060) {
	msg = "can not stat '%{filename}': %{errnostr}";
    } else if (message->code == 3600061) {
	msg = "%{dirname} is not a directory";
    } else if (message->code == 3600062) {
	msg = "can not stat '%{dirname}': %{errnostr}";
    } else if (message->code == 3600063) {
	msg = "can not %{noun} '%{ilename}': %{errnostr} (ruid:%{ruid} euid:%{euid})";
    } else if (message->code == 3600064) {
	msg = "%{filename} %{adjective} (ruid:%{ruid} euid:%{euid})";
    } else if (message->code == 3600065) {
	msg = "%{filename} is not owned by root";
    } else if (message->code == 3600066) {
	msg = "%{filename} is not SUID root";
    } else if (message->code == 3600067) {
	msg = "can not stat '%{filename}': %{errnostr}";
    } else if (message->code == 3600068) {
	msg = "cannot get filesystem usage for '%{dirname}: %{errnostr}";
    } else if (message->code == 3600069) {
	msg = "dir '%{dirname}' needs %{size:required}, has nothing available";
    } else if (message->code == 3600070) {
	msg = "dir '%{dirname}' needs %{size:required}, only has %{size:avail} available";
    } else if (message->code == 3600071) {
	msg = "dir '%{dirname}' has more than %{size:avail} available";
    } else if (message->code == 3600072) {
	msg = "DUMP program not available";
    } else if (message->code == 3600073) {
	msg = "RESTORE program not available";
    } else if (message->code == 3600074) {
	msg = "VDUMP program not available";
    } else if (message->code == 3600075) {
	msg = "VRESTORE program not available";
    } else if (message->code == 3600076) {
	msg = "XFSDUMP program not available";
    } else if (message->code == 3600077) {
	msg = "XFSRESTORE program not available";
    } else if (message->code == 3600078) {
	msg = "VXDUMP program not available";
    } else if (message->code == 3600079) {
	msg = "VXRESTORE program not available";
    } else if (message->code == 3600080) {
	msg = "GNUTAR program not available";
    } else if (message->code == 3600081) {
	msg = "SMBCLIENT program not available";
    } else if (message->code == 3600082) {
	msg = "/etc/amandapass is world readable!";
    } else if (message->code == 3600083) {
	msg = "/etc/amandapass is readable, but not by all";
    } else if (message->code == 3600084) {
	msg = "unable to stat /etc/amandapass: %{errnostr}";
    } else if (message->code == 3600085) {
	msg = "unable to open /etc/amandapass: %{errnostr}";
    } else if (message->code == 3600086) {
	msg = "dump will not be able to create the /etc/dumpdates file: %{errnostr}";
    } else if (message->code == 3600087) {
	msg = "%{device}: samba access error: %{errmsg}";
    } else if (message->code == 3600088) {
	msg = "file/dir '%{filename}' is not owned by root";
    } else if (message->code == 3600089) {
	msg = "file/dir '%{filename}' is writable by everyone";
    } else if (message->code == 3600090) {
	msg = "file/dir '%{filename}' is writable by the group";
    } else if (message->code == 3700000) {
	msg = "%{disk}";
    } else if (message->code == 3700001) {
	msg = "amgtar version %{version}";
    } else if (message->code == 3700002) {
	msg = "amgtar gtar-version %{gtar-version}";
    } else if (message->code == 3700003) {
	msg = "Can't get %{gtar-path} version";
    } else if (message->code == 3700004) {
	msg = "amgtar";
    } else if (message->code == 3700005) {
	msg = "GNUTAR program not available";
    } else if (message->code == 3700006) {
	msg = "No GNUTAR-LISTDIR";
    } else if (message->code == 3700007) {
	msg = "bad ONE-FILE-SYSTEM property value '%{value}'";
    } else if (message->code == 3700008) {
	msg = "bad SPARSE property value '%{value}'";
    } else if (message->code == 3700009) {
	msg = "bad ATIME-PRESERVE property value '%{value}'";
    } else if (message->code == 3700010) {
	msg = "bad CHECK-DEVICE property value '%{value}'";
    } else if (message->code == 3700011) {
	msg = "bad NO-UNQUOTE property value '%{value}'";
    } else if (message->code == 3700012) {
	msg = "Can't open disk '%{diskname}': %{errnostr}";
    } else if (message->code == 3700013) {
	msg = "Cannot stat the disk '%{diskname}': %{errnostr}";

    } else if (message->code == 4600000) {
	msg = "%{errmsg}";
    } else if (message->code == 4600001) {
	msg = "ERROR %{errmsg}";

    } else {
	msg = "no message for code '%{code}'";
    }

    result = fix_message_string(message, want_quoted, msg);
    if (want_quoted) {
	if (result) {
	    message->quoted_msg = g_string_free(result, FALSE);
	}
    } else {
	if (result) {
	    message->msg = g_string_free(result, FALSE);
	}
	result = fix_message_string(message, FALSE, hint);
	if (result) {
	    message->hint = g_string_free(result, FALSE);
	}
    }
}
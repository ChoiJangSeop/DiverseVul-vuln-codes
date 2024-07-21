void OverlaySettings::save(QSettings* settings_ptr) {
	OverlaySettings def;

	SAVELOAD(bEnable, "enable");

	SAVELOAD(osShow, "show");
	SAVELOAD(bAlwaysSelf, "alwaysself");
	SAVELOAD(uiActiveTime, "activetime");
	SAVELOAD(osSort, "sort");
	SAVELOAD(fX, "x");
	SAVELOAD(fY, "y");
	SAVELOAD(fZoom, "zoom");
	SAVELOAD(uiColumns, "columns");

	settings_ptr->beginReadArray(QLatin1String("states"));
	for (int i=0; i<4; ++i) {
		settings_ptr->setArrayIndex(i);
		SAVELOAD(qcUserName[i], "color");
		SAVELOAD(fUser[i], "opacity");
	}
	settings_ptr->endArray();

	SAVELOAD(qfUserName, "userfont");
	SAVELOAD(qfChannel, "channelfont");
	SAVELOAD(qcChannel, "channelcolor");
	SAVELOAD(qfFps, "fpsfont");
	SAVELOAD(qcFps, "fpscolor");

	SAVELOAD(fBoxPad, "padding");
	SAVELOAD(fBoxPenWidth, "penwidth");
	SAVELOAD(qcBoxPen, "pencolor");
	SAVELOAD(qcBoxFill, "fillcolor");

	SAVELOAD(bUserName, "usershow");
	SAVELOAD(bChannel, "channelshow");
	SAVELOAD(bMutedDeafened, "mutedshow");
	SAVELOAD(bAvatar, "avatarshow");
	SAVELOAD(bBox, "boxshow");
	SAVELOAD(bFps, "fpsshow");

	SAVELOAD(fUserName, "useropacity");
	SAVELOAD(fChannel, "channelopacity");
	SAVELOAD(fMutedDeafened, "mutedopacity");
	SAVELOAD(fAvatar, "avataropacity");
	SAVELOAD(fFps, "fpsopacity");

	SAVELOAD(qrfUserName, "userrect");
	SAVELOAD(qrfChannel, "channelrect");
	SAVELOAD(qrfMutedDeafened, "mutedrect");
	SAVELOAD(qrfAvatar, "avatarrect");
	SAVELOAD(qrfFps, "fpsrect");

	SAVEFLAG(qaUserName, "useralign");
	SAVEFLAG(qaChannel, "channelalign");
	SAVEFLAG(qaMutedDeafened, "mutedalign");
	SAVEFLAG(qaAvatar, "avataralign");

	settings_ptr->setValue(QLatin1String("usewhitelist"), bUseWhitelist);
	settings_ptr->setValue(QLatin1String("blacklist"), qslBlacklist);
	settings_ptr->setValue(QLatin1String("whitelist"), qslWhitelist);
}
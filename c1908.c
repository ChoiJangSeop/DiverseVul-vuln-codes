	bool ChanPage(CWebSock& WebSock, CTemplate& Tmpl, CIRCNetwork* pNetwork, CChan* pChan = NULL) {
		CSmartPtr<CWebSession> spSession = WebSock.GetSession();
		Tmpl.SetFile("add_edit_chan.tmpl");
		CUser* pUser = pNetwork->GetUser();

		if (!pUser) {
			WebSock.PrintErrorPage("That user doesn't exist");
			return true;
		}

		if (!WebSock.GetParam("submitted").ToUInt()) {
			Tmpl["User"] = pUser->GetUserName();
			Tmpl["Network"] = pNetwork->GetName();

			if (pChan) {
				Tmpl["Action"] = "editchan";
				Tmpl["Edit"] = "true";
				Tmpl["Title"] = "Edit Channel" + CString(" [" + pChan->GetName() + "]") + " of Network [" + pNetwork->GetName() + "] of User [" + pNetwork->GetUser()->GetUserName() + "]";
				Tmpl["ChanName"] = pChan->GetName();
				Tmpl["BufferCount"] = CString(pChan->GetBufferCount());
				Tmpl["DefModes"] = pChan->GetDefaultModes();
				Tmpl["Key"] = pChan->GetKey();

				if (pChan->InConfig()) {
					Tmpl["InConfig"] = "true";
				}
			} else {
				Tmpl["Action"] = "addchan";
				Tmpl["Title"] = "Add Channel" + CString(" for User [" + pUser->GetUserName() + "]");
				Tmpl["BufferCount"] = CString(pUser->GetBufferCount());
				Tmpl["DefModes"] = CString(pUser->GetDefaultChanModes());
				Tmpl["InConfig"] = "true";
			}

			// o1 used to be AutoCycle which was removed

			CTemplate& o2 = Tmpl.AddRow("OptionLoop");
			o2["Name"] = "autoclearchanbuffer";
			o2["DisplayName"] = "Auto Clear Chan Buffer";
			o2["Tooltip"] = "Automatically Clear Channel Buffer After Playback";
			if ((pChan && pChan->AutoClearChanBuffer()) || (!pChan && pUser->AutoClearChanBuffer())) { o2["Checked"] = "true"; }

			CTemplate& o3 = Tmpl.AddRow("OptionLoop");
			o3["Name"] = "detached";
			o3["DisplayName"] = "Detached";
			if (pChan && pChan->IsDetached()) { o3["Checked"] = "true"; }

			CTemplate& o4 = Tmpl.AddRow("OptionLoop");
			o4["Name"] = "disabled";
			o4["DisplayName"] = "Disabled";
			if (pChan && pChan->IsDisabled()) { o4["Checked"] = "true"; }

			FOR_EACH_MODULE(i, pNetwork) {
				CTemplate& mod = Tmpl.AddRow("EmbeddedModuleLoop");
				mod.insert(Tmpl.begin(), Tmpl.end());
				mod["WebadminAction"] = "display";
				if ((*i)->OnEmbeddedWebRequest(WebSock, "webadmin/channel", mod)) {
					mod["Embed"] = WebSock.FindTmpl(*i, "WebadminChan.tmpl");
					mod["ModName"] = (*i)->GetModName();
				}
			}

			return true;
		}

		CString sChanName = WebSock.GetParam("name").Trim_n();

		if (!pChan) {
			if (sChanName.empty()) {
				WebSock.PrintErrorPage("Channel name is a required argument");
				return true;
			}

			if (pNetwork->FindChan(sChanName.Token(0))) {
				WebSock.PrintErrorPage("Channel [" + sChanName.Token(0) + "] already exists");
				return true;
			}

			pChan = new CChan(sChanName, pNetwork, true);
			pNetwork->AddChan(pChan);
		}

		pChan->SetBufferCount(WebSock.GetParam("buffercount").ToUInt(), spSession->IsAdmin());
		pChan->SetDefaultModes(WebSock.GetParam("defmodes"));
		pChan->SetInConfig(WebSock.GetParam("save").ToBool());
		pChan->SetAutoClearChanBuffer(WebSock.GetParam("autoclearchanbuffer").ToBool());
		pChan->SetKey(WebSock.GetParam("key"));

		bool bDetached = WebSock.GetParam("detached").ToBool();
		if (pChan->IsDetached() != bDetached) {
			if (bDetached) {
				pChan->DetachUser();
			} else {
				pChan->AttachUser();
			}
		}

		bool bDisabled = WebSock.GetParam("disabled").ToBool();
		if (bDisabled)
			pChan->Disable();
		else
			pChan->Enable();

		CTemplate TmplMod;
		TmplMod["User"] = pUser->GetUserName();
		TmplMod["ChanName"] = sChanName;
		TmplMod["WebadminAction"] = "change";
		FOR_EACH_MODULE(it, pNetwork) {
			(*it)->OnEmbeddedWebRequest(WebSock, "webadmin/channel", TmplMod);
		}

		if (!CZNC::Get().WriteConfig()) {
			WebSock.PrintErrorPage("Channel added/modified, but config was not written");
			return true;
		}

		WebSock.Redirect("editnetwork?user=" + pUser->GetUserName().Escape_n(CString::EURL) + "&network=" + pNetwork->GetName().Escape_n(CString::EURL));
		return true;
	}
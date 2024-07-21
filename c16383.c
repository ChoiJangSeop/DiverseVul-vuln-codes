char *SSL_CIPHER_description(const SSL_CIPHER *cipher, char *buf, int len)
	{
	int is_export,pkl,kl;
	const char *ver,*exp_str;
	const char *kx,*au,*enc,*mac;
	unsigned long alg_mkey,alg_auth,alg_enc,alg_mac,alg_ssl,alg2;
#ifdef KSSL_DEBUG
	static const char *format="%-23s %s Kx=%-8s Au=%-4s Enc=%-9s Mac=%-4s%s AL=%lx/%lx/%lx/%lx/%lx\n";
#else
	static const char *format="%-23s %s Kx=%-8s Au=%-4s Enc=%-9s Mac=%-4s%s\n";
#endif /* KSSL_DEBUG */

	alg_mkey = cipher->algorithm_mkey;
	alg_auth = cipher->algorithm_auth;
	alg_enc = cipher->algorithm_enc;
	alg_mac = cipher->algorithm_mac;
	alg_ssl = cipher->algorithm_ssl;

	alg2=cipher->algorithm2;

	is_export=SSL_C_IS_EXPORT(cipher);
	pkl=SSL_C_EXPORT_PKEYLENGTH(cipher);
	kl=SSL_C_EXPORT_KEYLENGTH(cipher);
	exp_str=is_export?" export":"";
	
	if (alg_ssl & SSL_SSLV2)
		ver="SSLv2";
	else if (alg_ssl & SSL_SSLV3)
		ver="SSLv3";
	else
		ver="unknown";

	switch (alg_mkey)
		{
	case SSL_kRSA:
		kx=is_export?(pkl == 512 ? "RSA(512)" : "RSA(1024)"):"RSA";
		break;
	case SSL_kDHr:
		kx="DH/RSA";
		break;
	case SSL_kDHd:
		kx="DH/DSS";
		break;
        case SSL_kKRB5:
		kx="KRB5";
		break;
	case SSL_kEDH:
		kx=is_export?(pkl == 512 ? "DH(512)" : "DH(1024)"):"DH";
		break;
	case SSL_kECDHr:
		kx="ECDH/RSA";
		break;
	case SSL_kECDHe:
		kx="ECDH/ECDSA";
		break;
	case SSL_kEECDH:
		kx="ECDH";
		break;
	case SSL_kPSK:
		kx="PSK";
		break;
	default:
		kx="unknown";
		}

	switch (alg_auth)
		{
	case SSL_aRSA:
		au="RSA";
		break;
	case SSL_aDSS:
		au="DSS";
		break;
	case SSL_aDH:
		au="DH";
		break;
        case SSL_aKRB5:
		au="KRB5";
		break;
        case SSL_aECDH:
		au="ECDH";
		break;
	case SSL_aNULL:
		au="None";
		break;
	case SSL_aECDSA:
		au="ECDSA";
		break;
	case SSL_aPSK:
		au="PSK";
		break;
	default:
		au="unknown";
		break;
		}

	switch (alg_enc)
		{
	case SSL_DES:
		enc=(is_export && kl == 5)?"DES(40)":"DES(56)";
		break;
	case SSL_3DES:
		enc="3DES(168)";
		break;
	case SSL_RC4:
		enc=is_export?(kl == 5 ? "RC4(40)" : "RC4(56)")
		  :((alg2&SSL2_CF_8_BYTE_ENC)?"RC4(64)":"RC4(128)");
		break;
	case SSL_RC2:
		enc=is_export?(kl == 5 ? "RC2(40)" : "RC2(56)"):"RC2(128)";
		break;
	case SSL_IDEA:
		enc="IDEA(128)";
		break;
	case SSL_eNULL:
		enc="None";
		break;
	case SSL_AES128:
		enc="AES(128)";
		break;
	case SSL_AES256:
		enc="AES(256)";
		break;
	case SSL_CAMELLIA128:
		enc="Camellia(128)";
		break;
	case SSL_CAMELLIA256:
		enc="Camellia(256)";
		break;
	case SSL_SEED:
		enc="SEED(128)";
		break;
	default:
		enc="unknown";
		break;
		}

	switch (alg_mac)
		{
	case SSL_MD5:
		mac="MD5";
		break;
	case SSL_SHA1:
		mac="SHA1";
		break;
	default:
		mac="unknown";
		break;
		}

	if (buf == NULL)
		{
		len=128;
		buf=OPENSSL_malloc(len);
		if (buf == NULL) return("OPENSSL_malloc Error");
		}
	else if (len < 128)
		return("Buffer too small");

#ifdef KSSL_DEBUG
	BIO_snprintf(buf,len,format,cipher->name,ver,kx,au,enc,mac,exp_str,alg_mkey,alg_auth,alg_enc,alg_mac,alg_ssl);
#else
	BIO_snprintf(buf,len,format,cipher->name,ver,kx,au,enc,mac,exp_str);
#endif /* KSSL_DEBUG */
	return(buf);
	}
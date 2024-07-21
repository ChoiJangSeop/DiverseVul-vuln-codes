idn2_strerror (int rc)
{
  bindtextdomain (PACKAGE, LOCALEDIR);

  switch (rc)
    {
    case IDN2_OK: return _("success");
    case IDN2_MALLOC: return _("out of memory");
    case IDN2_NO_CODESET: return _("could not determine locale encoding format");
    case IDN2_ICONV_FAIL: return _("could not convert string to UTF-8");
    case IDN2_ENCODING_ERROR: return _("string encoding error");
    case IDN2_NFC: return _("string could not be NFC normalized");
    case IDN2_PUNYCODE_BAD_INPUT: return _("string contains invalid punycode data");
    case IDN2_PUNYCODE_BIG_OUTPUT: return _("punycode encoded data will be too large");
    case IDN2_PUNYCODE_OVERFLOW: return _("punycode conversion resulted in overflow");
    case IDN2_TOO_BIG_DOMAIN: return _("domain name longer than 255 characters");
    case IDN2_TOO_BIG_LABEL: return _("domain label longer than 63 characters");
    case IDN2_INVALID_ALABEL: return _("input A-label is not valid");
    case IDN2_UALABEL_MISMATCH: return _("input A-label and U-label does not match");
    case IDN2_NOT_NFC: return _("string is not in Unicode NFC format");
    case IDN2_2HYPHEN: return _("string contains forbidden two hyphens pattern");
    case IDN2_HYPHEN_STARTEND: return _("string start/ends with forbidden hyphen");
    case IDN2_LEADING_COMBINING: return _("string contains a forbidden leading combining character");
    case IDN2_DISALLOWED: return _("string contains a disallowed character");
    case IDN2_CONTEXTJ: return _("string contains a forbidden context-j character");
    case IDN2_CONTEXTJ_NO_RULE: return _("string contains a context-j character with null rule");
    case IDN2_CONTEXTO: return _("string contains a forbidden context-o character");
    case IDN2_CONTEXTO_NO_RULE: return _("string contains a context-o character with null rule");
    case IDN2_UNASSIGNED: return _("string contains unassigned code point");
    case IDN2_BIDI: return _("string has forbidden bi-directional properties");
    case IDN2_DOT_IN_LABEL: return _("domain label has forbidden dot (TR46)");
    case IDN2_INVALID_TRANSITIONAL: return _("domain label has character forbidden in transitional mode (TR46)");
    case IDN2_INVALID_NONTRANSITIONAL: return _("domain label has character forbidden in non-transitional mode (TR46)");
    default: return _("Unknown error");
    }
}
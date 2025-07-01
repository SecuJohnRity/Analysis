/*
 * Shared functions for Syscheck events decoding
 * Copyright (C) 2015, Wazuh Inc.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#ifdef WIN32

#include "utf8_winapi_wrapper.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <io.h>
#include <direct.h>

wchar_t *utf8_to_wide(const char *input) {
    if (!input) {
        return NULL;
    }

    UINT codepage = w_utf8_valid(input) ? CP_UTF8 : CP_ACP;

    int len = MultiByteToWideChar(codepage, 0, input, -1, NULL, 0);
    if (len == 0) {
        return NULL;
    }

    wchar_t *output = NULL;
    os_calloc(len, sizeof(wchar_t), output);

    if (!MultiByteToWideChar(codepage, 0, input, -1, output, len)) {
        os_free(output);
        return NULL;
    }

    return output;
}

char *wide_to_utf8(const wchar_t *input) {
    if (!input) {
        return NULL;
    }

    int len = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
    if (len == 0) {
        return NULL;
    }

    char *output = NULL;
    os_calloc(len, sizeof(char), output);

    if (!WideCharToMultiByte(CP_UTF8, 0, input, -1, output, len, NULL, NULL)) {
        os_free(output);
        return NULL;
    }

    return output;
}


int utf8_stat64(const char * pathname, struct _stat64 * statbuf) {
    wchar_t *wpath = utf8_to_wide(pathname);
    if (!wpath) {
        errno = ENOMEM;
        return -1;
    }

    int result = _wstat64(wpath, statbuf);

    os_free(wpath);
    return result;
}

HANDLE utf8_CreateFile(const char *utf8_path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa, DWORD creation, DWORD flags, HANDLE h_template) {
    wchar_t *wpath = utf8_to_wide(utf8_path);
    if (!wpath) {
        return INVALID_HANDLE_VALUE;
    }

    HANDLE h = CreateFileW(wpath, access, share, sa, creation, flags, h_template);

    os_free(wpath);
    return h;
}

DWORD utf8_GetFileAttributes(const char *utf8_path) {
    wchar_t *wpath = utf8_to_wide(utf8_path);
    if (!wpath) {
        return INVALID_FILE_ATTRIBUTES;
    }

    DWORD attr = GetFileAttributesW(wpath);

    os_free(wpath);
    return attr;
}

BOOL utf8_GetFileSecurity(const char *utf8_path, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd, DWORD len, LPDWORD needed) {
    wchar_t *wpath = utf8_to_wide(utf8_path);
    if (!wpath) {
        return FALSE;
    }

    BOOL ok = GetFileSecurityW(wpath, si, psd, len, needed);

    os_free(wpath);
    return ok;
}

DWORD utf8_GetNamedSecurityInfo(const char *utf8_path, SE_OBJECT_TYPE obj_type, SECURITY_INFORMATION si, PSID *owner, PSID *group, PACL *dacl, PACL *sacl, PSECURITY_DESCRIPTOR *psd) {
    wchar_t *wpath = utf8_to_wide(utf8_path);
    if (!wpath) {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD res = GetNamedSecurityInfoW(wpath, obj_type, si, owner, group, dacl, sacl, psd);

    os_free(wpath);
    return res;
}

DWORD utf8_SetNamedSecurityInfo(const char *utf8_path, SE_OBJECT_TYPE obj_type, SECURITY_INFORMATION si, PSID owner, PSID group, PACL dacl, PACL sacl) {
    wchar_t *wpath = utf8_to_wide(utf8_path);
    if (!wpath) {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD res = SetNamedSecurityInfoW(wpath, obj_type, si, owner, group, dacl, sacl);

    os_free(wpath);
    return res;
}

#endif

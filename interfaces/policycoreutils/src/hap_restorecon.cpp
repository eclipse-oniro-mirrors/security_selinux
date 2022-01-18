/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hap_restorecon.h"
#include "callbacks.h"
#include "selinux_error.h"
#include "selinux_log.h"
#include <fstream>
#include <include/fts.h>
#include <iostream>
#include <selinux/label.h>
#include <selinux/restorecon.h>
#include <selinux_internal.h>
#include <sstream>
#include <cstring>
#include <string>
#include <vector>

using namespace Selinux;

namespace {
static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, SECURITY_DOMAIN, "Selinux"};
static const std::string SEHAP_CONTEXTS_FILE = "/system/etc/selinux/targeted/contexts/sehap_contexts";
static const std::string APL_PREFIX = "apl=";
static const std::string NAME_PREFIX = "name=";
static const std::string DOMAIN_PREFIX = "domain=";
static const std::string TYPE_PREFIX = "type=";
static const std::string PATH_PREFIX = "/data/app";

static pthread_once_t FC_ONCE = PTHREAD_ONCE_INIT;
} // namespace

struct selabel_handle *HapContext::fileContextsHandle = nullptr;
std::unordered_map<std::string, struct SehapContext> HapContext::sehapContextsBuff;

HapContext::HapContext() {}

HapContext::~HapContext() {}

struct selabel_handle *SelinuxRestoreconHandle()
{
    struct selinux_opt selinuxOpts[] = {
        {SELABEL_OPT_VALIDATE, NULL},
        {SELABEL_OPT_PATH, NULL},
        {SELABEL_OPT_DIGEST, NULL},
    };

    return selabel_open(SELABEL_CTX_FILE, selinuxOpts, 1);
}

static bool CouldSkip(const std::string &line)
{
    if (line.size() <= 10) {
        return true;
    }
    int i = 0;
    while (isspace(line[i++]))
        ;
    if (line[i] == '#') {
        return true;
    }
    if (line.find(APL_PREFIX) == line.npos) {
        return true;
    }
    return false;
}

static struct SehapContext DecodeString(std::string &line)
{
    std::stringstream input(line);
    std::string tmp;
    struct SehapContext contextBuff;
    bool aplVisit = false;
    bool nameVisit = false;
    bool domainVisit = false;
    bool typeVisit = false;

    while (input >> tmp) {
        if (!aplVisit && (tmp.find(APL_PREFIX) != tmp.npos)) {
            contextBuff.apl = tmp.substr(tmp.find(APL_PREFIX) + APL_PREFIX.size());
            aplVisit = true;
        } else if (!nameVisit && (tmp.find(NAME_PREFIX) != tmp.npos)) {
            contextBuff.name = tmp.substr(tmp.find(NAME_PREFIX) + NAME_PREFIX.size());
            nameVisit = true;
        } else if (!domainVisit && (tmp.find(DOMAIN_PREFIX) != tmp.npos)) {
            contextBuff.domain = tmp.substr(tmp.find(DOMAIN_PREFIX) + DOMAIN_PREFIX.size());
            domainVisit = true;
        } else if (!typeVisit && (tmp.find(TYPE_PREFIX) != tmp.npos)) {
            contextBuff.type = tmp.substr(tmp.find(TYPE_PREFIX) + TYPE_PREFIX.size());
            typeVisit = true;
        }
    }

    return contextBuff;
}

void HapContext::RestoreconInit()
{
    if (fileContextsHandle == nullptr) {
        fileContextsHandle = SelinuxRestoreconHandle();
    }
}

void HapContext::HapContextsClear()
{
    if (!sehapContextsBuff.empty()) {
        sehapContextsBuff.clear();
    }
}

bool HapContext::HapContextsLoad()
{
    // load sehap_contexts file
    std::ifstream contextsFile(SEHAP_CONTEXTS_FILE);
    if (contextsFile) {
        int lineNum = 0;
        std::string line;
        while (getline(contextsFile, line)) {
            lineNum++;
            if (CouldSkip(line))
                continue;
            struct SehapContext tmpContext = DecodeString(line);
            if (!tmpContext.apl.empty()) {
                sehapContextsBuff.emplace(tmpContext.apl + tmpContext.name, tmpContext);
            } else {
                SELINUX_LOG_INFO(LABEL, "hap_contexts read fail in line %{public}d", lineNum);
            }
        }
    } else {
        SELINUX_LOG_ERROR(LABEL, "Load hap_contexts fail, no such file: %{public}s", SEHAP_CONTEXTS_FILE.c_str());
        return false;
    }
    SELINUX_LOG_INFO(LABEL, "Load hap_contexts succes: %{public}s", SEHAP_CONTEXTS_FILE.c_str());
    contextsFile.close();
    return true;
}

int HapContext::HapContextsLookup(bool isDomain, const std::string &apl, const std::string &packageName, context_t con)
{
    if (sehapContextsBuff.empty()) {
        if (!HapContextsLoad()) {
            return -SELINUX_CONTEXTS_LOAD_ERR;
        }
    }

    auto iter = sehapContextsBuff.find(std::string(apl) + std::string(packageName));
    if (iter != sehapContextsBuff.end()) {
        std::string type = "";
        if (isDomain) {
            type = iter->second.domain;
        } else {
            type = iter->second.type;
        }
        if (context_type_set(con, type.c_str())) {
            SELINUX_LOG_ERROR(LABEL, "Set type for %{public}s fail", type.c_str());
            return -SELINUX_TYPE_SET_ERR;
        }
        return SELINUX_SUCC;
    }
    return -SELINUX_KEY_NOT_FOUND;
}

int HapContext::HapLabelLookup(const std::string &apl, const std::string &packageName, char **secontextPtr)
{
    if (apl.size() == 0 || secontextPtr == nullptr) {
        return -SELINUX_ARG_INVALID;
    }
    char *secontext = *secontextPtr;
    context_t con = context_new(secontext);
    if (con == nullptr) {
        return -SELINUX_PTR_NULL;
    }

    int res = HapContextsLookup(false, apl, packageName, con);
    if (res < 0) {
        context_free(con);
        return res;
    }

    secontext = context_str(con);
    if (secontext == nullptr) {
        context_free(con);
        return -SELINUX_PTR_NULL;
    }

    // if new contexts is same as old
    if (!strcmp(secontext, *secontextPtr)) {
        context_free(con);
        return SELINUX_SUCC;
    }

    // check whether the context is valid
    if (security_check_context(secontext) < 0) {
        context_free(con);
        return -SELINUX_TYPE_INVALID;
    }

    freecon(*secontextPtr);
    *secontextPtr = strdup(secontext);
    if (!(*secontextPtr)) {
        context_free(con);
        return -SELINUX_PTR_NULL;
    }

    context_free(con);
    return SELINUX_SUCC;
}

int HapContext::RestoreconSb(const std::string &pathname, const struct stat *sb, const std::string &apl,
                             const std::string &packageName)
{
    char *secontext = nullptr;
    char *oldSecontext = nullptr;

    if (selabel_lookup(fileContextsHandle, &secontext, pathname.c_str(), sb->st_mode) < 0) {
        return SELINUX_SUCC;
    }

    if (lgetfilecon(pathname.c_str(), &oldSecontext) < 0) {
        freecon(secontext);
        freecon(oldSecontext);
        return -SELINUX_CONTEXTS_NOT_FOUND;
    }

    int res = HapLabelLookup(apl, packageName, &secontext);
    if (res < 0) {
        freecon(secontext);
        freecon(oldSecontext);
        return res;
    }

    if (strcmp(oldSecontext, secontext)) {
        if (lsetfilecon(pathname.c_str(), secontext) < 0) {
            freecon(secontext);
            freecon(oldSecontext);
            return -SELINUX_CONTEXTS_NOT_FOUND;
        }
    }

    freecon(secontext);
    freecon(oldSecontext);
    return SELINUX_SUCC;
}

int HapContext::HapFileRestorecon(std::vector<std::string> &pathNameOrig, const std::string &apl,
                                  const std::string &packageName, unsigned int flags)
{
    if (pathNameOrig.empty()) {
        return -SELINUX_PATH_INVAILD;
    }
    int res = SELINUX_SUCC;
    for (auto pathname : pathNameOrig) {
        res |= HapFileRestorecon(pathname.c_str(), apl, packageName, flags);
    }
    return res;
}

int HapContext::HapFileRestorecon(const std::string &pathNameOrig, const std::string &apl,
                                  const std::string &packageName, unsigned int flags)
{
    if (is_selinux_enabled() < 1) {
        SELINUX_LOG_INFO(LABEL, "Selinux not enbaled");
        return SELINUX_SUCC;
    }

    if (std::string(pathNameOrig).compare(0, PATH_PREFIX.size(), PATH_PREFIX) != 0) {
        return -SELINUX_PATH_INVAILD;
    }

    // get file_contexts handle
    __selinux_once(FC_ONCE, RestoreconInit);
    if (fileContextsHandle == nullptr) {
        SELINUX_LOG_ERROR(LABEL, "Cannot get file context handle: %{public}s", strerror(errno));
        return -SELINUX_PTR_NULL;
    }

    struct stat sb;
    char realPath[PATH_MAX];
    if (realpath(pathNameOrig.c_str(), realPath) == nullptr) {
        return -SELINUX_PATH_INVAILD;
    }

    bool recurse = (flags & SELINUX_HAP_RESTORECON_RECURSE) ? true : false;
    if (!recurse) {
        if (lstat(realPath, &sb) < 0) {
            return -SELINUX_FILE_INVAILD;
        }

        int res = RestoreconSb(realPath, &sb, apl, packageName);
        if (res < 0) {
            SELINUX_LOG_ERROR(LABEL, "RestoreconSb failed");
        }
        return res;
    }

    char *paths[2] = {NULL, NULL};
    paths[0] = (char *)realPath;
    int ftsFlags = FTS_PHYSICAL | FTS_NOCHDIR;
    FTS *fts = fts_open(paths, ftsFlags, NULL);
    if (fts == nullptr) {
        return -SELINUX_FILE_ERR;
    }

    FTSENT *ftsent = nullptr;
    int error = 0;
    while ((ftsent = fts_read(fts)) != NULL) {
        switch (ftsent->fts_info) {
            case FTS_DC:
                SELINUX_LOG_ERROR(LABEL, "Cycle on %{public}s", ftsent->fts_path);
                (void)fts_close(fts);
                return -SELINUX_FTS_ELOOP;
            case FTS_DP:
                continue;
            case FTS_DNR:
                SELINUX_LOG_ERROR(LABEL, "Read error on %{public}s, errorno: %{public}s", ftsent->fts_path,
                                  strerror(errno));
                fts_set(fts, ftsent, FTS_SKIP);
                continue;
            case FTS_ERR:
                SELINUX_LOG_ERROR(LABEL, "Error on %{public}s, errorno: %{public}s", ftsent->fts_path, strerror(errno));
                fts_set(fts, ftsent, FTS_SKIP);
                continue;
            case FTS_NS:
                SELINUX_LOG_ERROR(LABEL, "stat error on %{public}s, errorno: %{public}s", ftsent->fts_path,
                                  strerror(errno));
                fts_set(fts, ftsent, FTS_SKIP);
                continue;
            case FTS_D:
            default:
                error |= RestoreconSb(ftsent->fts_path, ftsent->fts_statp, apl, packageName);
                break;
        }
    }
    (void)fts_close(fts);
    return error;
}

int HapContext::HapDomainSetcontext(const std::string &apl, const std::string &packageName)
{
    if (is_selinux_enabled() < 1) {
        SELINUX_LOG_INFO(LABEL, "Selinux not enbaled");
        return SELINUX_SUCC;
    }

    char *typeContext = nullptr;
    if (getcon(&typeContext)) {
        return -SELINUX_GETCON_ERR;
    }

    context_t con = nullptr;
    con = context_new(typeContext);
    if (con == nullptr) {
        return -SELINUX_PTR_NULL;
    }
    char *oldTypeContext = typeContext;

    int res = HapContextsLookup(true, apl, packageName, con);
    if (res < 0) {
        freecon(oldTypeContext);
        context_free(con);
        return res;
    }

    typeContext = context_str(con);
    if (typeContext == nullptr) {
        freecon(oldTypeContext);
        context_free(con);
        return -SELINUX_PTR_NULL;
    }

    SELINUX_LOG_INFO(LABEL, "Hap type for %{public}s is changing from %{public}s to %{public}s", packageName.c_str(),
                     oldTypeContext, typeContext);

    if (security_check_context(typeContext) < 0) {
        freecon(oldTypeContext);
        context_free(con);
        return -SELINUX_TYPE_INVALID;
    }

    if (strcmp(typeContext, oldTypeContext)) {
        if (setcon(typeContext) < 0) {
            freecon(oldTypeContext);
            context_free(con);
            return -SELINUX_SETCON_ERR;
        }
    }
    SELINUX_LOG_INFO(LABEL, "Hap setcon finish for %{public}s", packageName.c_str());

    freecon(oldTypeContext);
    context_free(con);
    return SELINUX_SUCC;
}

// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include "windows.h"
#endif

// ---- constants & macros ------------------------------

#define NOTIFIER_NAME "bugsnag-win-c"
#define NOTIFIER_VERSION "0.4.0.0"
#define NOTIFIER_URL "https://github.com/spike0xff/bugsnag-win-c"

#define STRINGQ(x) #x
#define STRINGIZE(s) STRINGQ(s)

// ---- type declarations -------------------------------

// ---- forward function declarations -------------------

static void set_severity(BugsnagSeverity howbad);

static void replace_all(char *str, char old, char rep);

static int stage_in_notify_stages(void);

// copy source to buffer, adding escapes to make it a legal JSON string body
// AND if it won't fit, replace excess on the LEFT with '...'
static void escape_and_left_punch(char *buffer, size_t bufsize, const char* source);

static void update_device(void);

static void update_app_member(void);

static void update_user_member(void);

static void launch_curl(const char* args);

static const char* get_file_version(const char* filename);


// ---- static global variables -------------------------

static char apiKey[64];
char exePath[MAX_PATH];
static const char* severity = "";
static char encodedFilename[_MAX_PATH];
static char appVersion[16];
static char stages[256];
static char releaseStage[24];
static char appType[32];
static char userName[64];
static char userId[32];
static char userEmail[128];
static char user[256];
static char app[256];
static char device[128];
static char metadata[1024];
static char context[128];
static char groupingHash[128];

// ---- public API functions ----------------------------

void bugsnag_init(const char* key)
{
    strcpy_s(apiKey, sizeof apiKey, key);
    GetModuleFileName(NULL, exePath, MAX_PATH); // get this app's path
    bugsnag_set_user_name(NULL);            // default the user name
    bugsnag_set_release_stage(NULL);        // default the app releaseStage
    bugsnag_set_notify_stages(NULL);
    bugsnag_set_app_version(NULL);
    update_device();
}

void bugsnag_set_app_version(const char* version)
{
    if (!version || !*version) {
        version = get_file_version(exePath);
        if (!version) {
            version = "";
        }
    }
    strcpy_s(appVersion, sizeof appVersion, version);
    update_app_member();
}

void bugsnag_set_app_type(const char* typ)
{
    typ = typ ? typ : "unknown";
    strcpy_s(appType, sizeof appType, typ);
    update_app_member();
}

void bugsnag_set_release_stage(const char* stage)
{
    if (!stage || !*stage) {
        stage = getenv("BUGSNAG_STAGE");
        if (!stage) {
            stage = STRINGIZE(CONFIGURATION);
        }
    }
    strcpy_s(releaseStage, sizeof releaseStage, stage);
    _strlwr(releaseStage);
    update_app_member();
}

void bugsnag_set_notify_stages(const char* stage_list)
{
    if (stage_list) {
        strcpy_s(stages, sizeof stages - 1, stage_list);
        // double NUL at the end
        // or equivalently, ends with an empty string:
        strchr(stages, 0)[1] = (char)0;
        // turn commas into NULs to make later searching easier
        replace_all(stages, ',', '\0');
    }
}

void bugsnag_set_user_name(const char* name)
{
    if (!name || !*name) {
        // default to what OS tells us
        name = getenv("USERNAME");
        if (!name) name = "";
    }
    sprintf_s(userName, sizeof userName, "\"name\": \"%s\"", name);
    update_user_member();
}

void bugsnag_set_user_id(const char* id)
{
    if (id && *id) {
        sprintf_s(userId, sizeof userId, ", \"id\": \"%s\"", id);
    }
    else {
        userId[0] = (char)0;
    }
    update_user_member();
}

void bugsnag_set_user_email(const char* email)
{
    if (email && *email) {
        sprintf_s(userEmail, sizeof userEmail, ", \"email\": \"%s\"", email);
    }
    else {
        userEmail[0] = (char)0;
    }
    update_user_member();
}

void bugsnag_set_metadata(const char* meta)
{
    if (meta && *meta) {
        sprintf_s(metadata, sizeof metadata, ",\n    \"metaData\": %s", meta);
    }
    else {
        metadata[0] = (char)0;
    }
}

void bugsnag_set_context(const char* ctx)
{
    if (ctx && *ctx) {
        sprintf_s(context, sizeof context, ",\n    \"context\": \"%s\"", ctx);
    }
    else {
        context[0] = (char)0;
    }
}

void bugsnag_set_grouping_hash(const char* hash)
{
    if (hash && *hash) {
        sprintf_s(groupingHash, sizeof groupingHash, ",\n    \"context\": \"%s\"", hash);
    }
    else {
        groupingHash[0] = (char)0;
    }
}

int bugsnag_notify(BugsnagSeverity howbad, const char* errorClass, const char* message, const char* file, int lineNumber, const char* method)
{
    if (!apiKey[0]) {
        fprintf(stderr, "**bugsnag_notify called with no apiKey set\n");
        return 0;
    }
    if (!stage_in_notify_stages()) {
        return  1;
    }
    // prep any data needed for the payload
    if (!errorClass || !*errorClass) {
        errorClass = "error";
    }
    if (!message) message = "";

    // escape the filename as needed to be a JSON string:
    escape_and_left_punch(encodedFilename, sizeof encodedFilename, file);
    set_severity(howbad);

    // create the JSON payload file, writing literally:
    char tmpfile[L_tmpnam];
    tmpnam(tmpfile);
    FILE* payload = fopen(tmpfile, "wb");
    if (!payload) {
        // failure
        return 0;
    }

    // single-event payload template
    const char* payload_template =
        "{\n"
        "  \"apiKey\": \"%s\",\n"
        "  \"notifier\" : {\n"
        "    \"name\": \"%s\",\n"
        "    \"version\" : \"%s\",\n"
        "    \"url\" : \"%s\"\n"
        "  },\n"
        "  \"events\" : [{\n"
        "    \"payloadVersion\": \"2\",\n"
        "    \"exceptions\" : [{\n"
        "      \"errorClass\": \"%s\",\n"
        "      \"message\" : \"%s\",\n"
        "      \"stacktrace\" : [\n"        // starting with top of stack
        "        { \"file\": \"%s\", \"lineNumber\" : %d, \"method\" : \"%s\" }\n"
        "      ]\n"
        // optional context, groupingHash, severity, user, app, device, metadata:
        "    }]%s%s%s%s%s%s%s\n"
        "  }]\n"                            // end of events array
        "}";

    fprintf(payload, payload_template,
        apiKey,
        NOTIFIER_NAME, NOTIFIER_VERSION, NOTIFIER_URL,
        errorClass, message,
        encodedFilename, lineNumber, method,
        context,
        groupingHash,
        severity,
        user,
        app,
        device,
        metadata
    );
    fclose(payload);
    // reset all one-time notification settings:
    bugsnag_set_metadata(NULL);
    bugsnag_set_grouping_hash(NULL);

    // use cURL to POST the payload to Bugsnag
    // notes:
    // -v is 'verbose' - lots of stuff pops up in a console window
    // --data @file  strips out carriage returns and newlines when reading the file
    char curlArgs[300];
    //sprintf_s(curlArgs, sizeof curlArgs, "-v -H \"Content-type: application/json\" --data @\"%s\" http://notify.bugsnag.com", tmpfile);
    sprintf_s(curlArgs, sizeof curlArgs, "-H \"Content-type: application/json\" --data @\"%s\" http://notify.bugsnag.com", tmpfile);
    launch_curl(curlArgs);

    return 1;
}

// ---- private (helper) functions ----------------------

static void update_app_member()
{
    const char* app_template =
        ",\n"
        "    \"app\": {\n"
        "      \"version\": \"%s\",\n"
        "      \"releaseStage\": \"%s\",\n"
        "      \"type\": \"%s\"\n"
        "    }";

    if (appVersion[0] || releaseStage[0] || appType[0]) {
        sprintf_s(app, sizeof app, app_template, appVersion, releaseStage, appType);
    }
    else {
        app[0] = (char)0;
    }
}

void update_user_member(void)
{
    const char* user_template =
        ",\n"
        "    \"user\": { %s%s%s }";
    sprintf_s(user, sizeof user, user_template, userName, userId, userEmail);
}

void set_severity(BugsnagSeverity howbad)
{
    switch (howbad) {
    case BUGSNAG_ERROR:
        severity = "";
        break;
    case BUGSNAG_WARNING:
        severity = ",\n    \"severity\": \"warning\"";
        break;
    case BUGSNAG_INFO:
        severity = ",\n    \"severity\": \"info\"";
        break;
    }
}

static void replace_all(char *str, char old, char rep) {
    while ((str = strchr(str, old))) {
        *str++ = rep;
    }
}

static int stage_in_notify_stages(void)
{
    // if NO notify stages are defined, treat that as 'all'
    if (!stages[0]) {
        return TRUE;
    }
    const char* s = stages;
    while (*s) {
        if (0 == strcmp(s, releaseStage)) {
            return TRUE;
        }
        s += strlen(s) + 1;
    }
    return FALSE;
}

void escape_and_left_punch(char *buffer, size_t bufsize, const char* source)
{
    char* out = buffer;
    const char* full = buffer + bufsize - 2;
    while (1) {
        char c = *source++;
        if (c == '\\') {
            *out++ = c;
        }
        *out++ = c;
        if (!c) break;
        if (out >= full) {
            strcpy_s(buffer, sizeof buffer, "...");
            memmove(buffer+3, buffer + 32, (out - buffer) - 32);
            out = out + 3 - 32;
        }
    }
}

static const char* get_os_version(void)
{
    const char* version = get_file_version("kernel32.dll");
    return version ? version : "unknown";
}

static const char* get_file_version(const char* filename)
{
#ifdef _WIN32
    char verData[512];
    if (GetFileVersionInfo(filename, 0, sizeof verData, verData))
    {
        LPBYTE lpBuffer = NULL;
        UINT   size = 0;
        if (VerQueryValue(verData, "\\", (VOID FAR* FAR*)&lpBuffer, &size))
        {
            VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
            if (size != 0 && verInfo->dwSignature == 0xfeef04bd)
            {
                static char version[64] = "";
                sprintf_s(version, sizeof version, "%d.%d.%d.%d",
                    (verInfo->dwFileVersionMS >> 16) & 0xffff,
                    (verInfo->dwFileVersionMS >> 0) & 0xffff,
                    (verInfo->dwFileVersionLS >> 16) & 0xffff,
                    (verInfo->dwFileVersionLS >> 0) & 0xffff
                );
                return version;
            }
        }
    }
#endif
    return NULL;
}

static const char* get_host_name(void)
{
    const char* computerName = NULL;
#if defined(WIN32) || defined(_WIN32)
    computerName = getenv("COMPUTERNAME");
#else
    computerName = getenv("HOSTNAME");
#endif
    if (!computerName) {
        computerName = "unknown";
    }
    return computerName;
}

void update_device(void)
{
    const char* device_template =
        ",\n \"device\": { \"os\": \"%s\", \"osVersion\": \"%s\", \"hostname\": \"%s\" }";
    sprintf_s(device, sizeof device, device_template, "Windows", get_os_version(), get_host_name());
}

void launch_curl(const char* args)
{
    // we grabbed the path to the exe of the running process, during init.
    // take that apart to get the drive & directory
    static char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath(exePath, drive, dir, fname, ext);
    // then put it back together but with "curl.exe"
    static char curlPath[MAX_PATH];
    _makepath(curlPath, drive, dir, "curl", "exe");
    // append the cURL argument(s)
    static char command[1024];
    sprintf_s(command, sizeof command, "\"%s\" %s", curlPath, args);
    // and fire it all off
    static PROCESS_INFORMATION procInfo;
    memset(&procInfo, 0, sizeof procInfo);
    static STARTUPINFO startupInfo;
    memset(&startupInfo, 0, sizeof startupInfo);
    startupInfo.cb = sizeof startupInfo;
    BOOL bGood = CreateProcess(
        NULL,
        command,
        NULL, NULL,         // processAttributes, threadAttributes
        FALSE,              // inherit handles (nope)
        HIGH_PRIORITY_CLASS,    // get 'er done
        NULL,               // environment
        NULL,               // current dir for new process
        &startupInfo,
        &procInfo
    );
    // close handles when done with them = now.
    CloseHandle(procInfo.hProcess);
    CloseHandle(procInfo.hThread);
}
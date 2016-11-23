// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include "windows.h"
#endif

// ---- constants ---------------------------------------

#define NOTIFIER_NAME "bugsnag-win-c"
#define NOTIFIER_VERSION "0.3.0.0"
#define NOTIFIER_URL "https://github.com/spike0xff/bugsnag-win-c"

// ---- type declarations -------------------------------

// ---- forward function declarations -------------------

static void set_severity(BugsnagSeverity howbad);

// copy source to buffer, adding escapes to make it a legal JSON string body
// AND if it won't fit, replace excess on the LEFT with '...'
static void encode_and_left_punch(char *buffer, size_t bufsize, const char* source);

static void update_device(void);

static void update_app_member(void);

static void update_user_member(void);

static int launch_curl(const char* args);

// ---- static global variables -------------------------

static char apiKey[64];
static const char* severity = "";
static char encodedFilename[_MAX_PATH];
static char appVersion[16];
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
    bugsnag_set_user_name(NULL);            // default the user name
    update_device();
}

void bugsnag_set_app_version(const char* version)
{
    strcpy_s(appVersion, sizeof appVersion, version);
    update_app_member();
}

void bugsnag_set_app_type(const char* typ)
{
    strcpy_s(appType, sizeof appType, typ);
    update_app_member();
}

extern void bugsnag_set_app_release_stage(const char* stage)
{
    strcpy_s(releaseStage, sizeof releaseStage, stage);
    _strlwr(releaseStage);
    update_app_member();
}

void bugsnag_set_user_name(const char* name)
{
    if (!name || !*name) {
        // default to what OS tells us
        name = getenv("USERNAME");
    }
    if (!name) name = "";
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
    // prep any data needed for the payload
    if (!errorClass || !*errorClass) {
        errorClass = "error";
    }
    if (!message) message = "";

    // escape the filename as needed to be a JSON string:
    encode_and_left_punch(encodedFilename, sizeof encodedFilename, file);
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

    if (appVersion[0]) {
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

void encode_and_left_punch(char *buffer, size_t bufsize, const char* source)
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
    static char version[64] = "unknown";
#ifdef _WIN32
    char verData[512];
    if (GetFileVersionInfo("kernel32.dll", 0, sizeof verData, verData))
    {
        LPBYTE lpBuffer = NULL;
        UINT   size = 0;
        if (VerQueryValue(verData, "\\", (VOID FAR* FAR*)&lpBuffer, &size))
        {
            VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
            if (size != 0 && verInfo->dwSignature == 0xfeef04bd)
            {
                sprintf_s(version, sizeof version, "%d.%d.%d.%d",
                    (verInfo->dwFileVersionMS >> 16) & 0xffff,
                    (verInfo->dwFileVersionMS >> 0) & 0xffff,
                    (verInfo->dwFileVersionLS >> 16) & 0xffff,
                    (verInfo->dwFileVersionLS >> 0) & 0xffff
                );
            }
        }
    }
#endif
    return version;
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
        ",\n \"device\": { \"osVersion\": \"%s\", \"hostname\": \"%s\" }";
    sprintf_s(device, sizeof device, device_template, get_os_version(), get_host_name());
}

int launch_curl(const char* args)
{
    char szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, MAX_PATH);
    char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath(szPath, drive, dir, fname, ext);
    static char command[512];
    _makepath(command, drive, dir, "curl", "exe");
    strcat_s(command, sizeof command, " ");
    strcat_s(command, sizeof command, args);
    return system(command);
}
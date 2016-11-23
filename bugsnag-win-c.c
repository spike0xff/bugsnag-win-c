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
#define NOTIFIER_VERSION "0.2.0.0"
#define NOTIFIER_URL "https://github.com/spike0xff/bugsnag-win-c"

#define DEFAULT_SEVERITY "error"

// ---- type declarations -------------------------------

// ---- forward function declarations -------------------

// copy source to buffer, adding escapes to make it a legal JSON string body
// AND if it won't fit, replace excess on the LEFT with '...'
static void encode_and_left_punch(char *buffer, size_t bufsize, const char* source);

static void update_app_member(char* app, size_t appbytes);

static const char* get_os_version(void);

static int launch_curl(const char* args);

// ---- static global variables -------------------------

static char apiKey[64];
static char severity[32] = DEFAULT_SEVERITY;
static char encodedFilename[_MAX_PATH];
static char appVersion[16];
static char releaseStage[24];
static char appType[32];
static char user[256];
static char app[256];

// ---- public API functions ----------------------------

void bugsnag_set_api_key(const char* key)
{
    strcpy_s(apiKey, sizeof apiKey, key);
}

void bugsnag_set_app_version(const char* version)
{
    strcpy_s(appVersion, sizeof appVersion, version);
}

void bugsnag_set_app_type(const char* typ)
{
    strcpy_s(appType, sizeof appType, typ);
}

void bugsnag_set_user(const char* name, const char* id, const char* email)
{
    const char* user_template =
        "    \"user\": {\n"
        "      \"id\": \"%s\",\n"
        "      \"name\": \"%s\",\n"
        "      \"email\": \"%s\"\n"
        "    },\n";
    if (!name) name = "";
    if (!id) id = "";
    if (!email) email = "";
    // if at least one item is non-null...
    if (*name || *id || *email) {
        // define a user-block for subsequent notifications
        sprintf_s(user, sizeof user, user_template, id, name, email);
    }
    else {
        // omit the user object
        user[0] = (char)0;
    }
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
"      \"stacktrace\" : [{\n"
"        \"file\": \"%s\",\n"
"        \"lineNumber\" : %d,\n"
"        \"method\" : \"%s\"\n"
"      }]\n"
"    }],\n"
"%s"            // user member (may be null)
"%s"            // app member (may be null)
"    \"severity\": \"%s\"\n"      // note NO TRAILING COMMA
"  }]\n"
"}";

int bugsnag_notify(const char* errorClass, const char* message, const char* file, int lineNumber, const char* method)
{
    if (!apiKey[0]) {
        fprintf(stderr, "**bugsnag_notify called with no apiKey set\n");
        return 0;
    }
    if (!errorClass || !*errorClass) {
        errorClass = "error";
    }
    if (!message) message = "";

    // create the JSON payload file, writing literally:
    FILE* payload = fopen("bugsnag.json", "wb");
    if (!payload) {
        // failure
        return 0;
    }
    // escape the filename as needed to be a JSON string:
    encode_and_left_punch(encodedFilename, sizeof encodedFilename, file);
    // update the app: member
    update_app_member(app, sizeof app);
    fprintf(payload, payload_template,
        apiKey,
        NOTIFIER_NAME, NOTIFIER_VERSION, NOTIFIER_URL,
        errorClass, message,
        encodedFilename, lineNumber, method,
        user,
        app,
        severity
    );
    fclose(payload);

    // use cURL to POST the payload to Bugsnag
    launch_curl("-v -H \"Content-type: application/json\" --data @bugsnag.json http://notify.bugsnag.com");

    // reset all one-time optional notification settings:
    strcpy_s(severity, sizeof severity, DEFAULT_SEVERITY);
    return 1;
}

// ---- private (helper) functions ----------------------

static void update_app_member(char* app, size_t appbytes)
{
    const char* app_template =
        "    \"app\": {\n"
        "      \"version\": \"%s\",\n"
        "      \"releaseStage\": \"%s\",\n"
        "      \"type\": \"%s\"\n"
        "    },\n";
    if (appVersion[0]) {
        sprintf_s(app, appbytes, app_template, appVersion, releaseStage, appType);
    }
    else {
        app[0] = (char)0;
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
    return "unknown";
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
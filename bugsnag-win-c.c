// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---- constants ---------------------------------------

#define NOTIFIER_NAME "bugsnag-win-c"
#define NOTIFIER_VERSION "0.1.1.0"
#define NOTIFIER_URL "https://github.com/spike0xff/bugsnag-win-c"

#define DEFAULT_SEVERITY "error"

// ---- forward function declarations -------------------

static void encodeFilename(char *encoded, const char* file);

// example working cURL command:
// curl -H "Content-Type: application/json" --data-binary @minimal.json -v http://notify.bugsnag.com

// ---- static global variables -------------------------

static char apiKey[64];
static char severity[32] = DEFAULT_SEVERITY;
static char encodedFilename[_MAX_PATH];

// ---- public API functions ----------------------------

void bugsnag_set_apiKey(const char* key)
{
    strcpy_s(apiKey, sizeof apiKey, key);
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
"    }]\n"
"  }],\n"
"  \"severity\": \"%s\"\n"
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
    encodeFilename(encodedFilename, file);
    fprintf(payload, payload_template,
        apiKey,
        NOTIFIER_NAME, NOTIFIER_VERSION, NOTIFIER_URL,
        errorClass, message,
        encodedFilename, lineNumber, method,
        severity
    );
    fclose(payload);

    // use cURL to POST the payload to Bugsnag
    system("curl -v -H \"Content-type: application/json\" --data @bugsnag.json http://notify.bugsnag.com");
    strcpy_s(severity, sizeof severity, DEFAULT_SEVERITY);
    return 1;
}

// ---- private (helper) functions ----------------------

void encodeFilename(char *buffer, const char* file)
{
    char* out = buffer;
    const char* full = buffer + _MAX_PATH - 2;
    while (1) {
        char c = *file++;
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


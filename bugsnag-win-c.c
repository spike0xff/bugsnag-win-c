// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"
#include "string.h"

// example working cURL command:
// curl -H "Content-Type: application/json" --data-binary @minimal.json -v http://notify.bugsnag.com

static char apiKey[64];

void bugsnag_set_apiKey(const char* key)
{
    strcpy_s(apiKey, sizeof apiKey, key);
}
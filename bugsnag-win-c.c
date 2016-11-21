// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"
#include "string.h"

static char apiKey[64];

void bugsnag_set_apiKey(const char* key)
{
    strcpy_s(apiKey, sizeof apiKey, key);
}
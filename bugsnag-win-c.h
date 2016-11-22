#pragma once
#ifndef BUGSNAG_WIN_C_H
#define BUGSNAG_WIN_C_H
#ifdef __cplusplus
extern "C"
{
#endif

    // set the apiKey for your project on Bugsnag.
    extern void bugsnag_set_apiKey(const char* key);

    // notify Bugsnag of a single event
    extern int bugsnag_notify(const char* errorClass, const char* message, const char* file, int lineNumber, const char* method);

#ifdef __cplusplus
}
#endif
#endif


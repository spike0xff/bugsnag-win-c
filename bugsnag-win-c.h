#pragma once
#ifndef BUGSNAG_WIN_C_H
#define BUGSNAG_WIN_C_H
#ifdef __cplusplus
extern "C"
{
#endif

    // Set the apiKey for your project on Bugsnag.
    extern void bugsnag_set_api_key(const char* key);

    // Define the application version for subsequent notifications.
    // If app_version is set and an error is resolved in the dashboard, the
    // error will not unresolve until it is seen in a newer version.
    // version format is the usual a.b.c or a.b.c.d
    extern void bugsnag_set_app_version(const char* version);

    extern void bugsnag_set_app_type(const char* appType);

    // Define the current user of the application for subsequent notifications.
    // Any of the parameters can be passed as "" or NULL.
    extern void bugsnag_set_user(const char* name, const char* id, const char* email);

    // Notify Bugsnag of a single event
    extern int bugsnag_notify(const char* errorClass, const char* message, const char* file, int lineNumber, const char* method);

#ifdef __cplusplus
}
#endif
#endif


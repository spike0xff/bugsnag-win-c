#pragma once
#ifndef BUGSNAG_WIN_C_H
#define BUGSNAG_WIN_C_H
#ifdef __cplusplus
extern "C"
{
#endif
    // severity of a notification
    typedef enum { BUGSNAG_ERROR, BUGSNAG_WARNING, BUGSNAG_INFO } BugsnagSeverity;

    // Initialize Bugsnag and set the apiKey for your project.
    extern void bugsnag_init(const char* key);

    // Define the application version for subsequent notifications.
    // If app_version is set and an error is resolved in the dashboard, the
    // error will not unresolve until it is seen in a newer version.
    // version format is the usual a.b.c or a.b.c.d
    extern void bugsnag_set_app_version(const char* version);

    extern void bugsnag_set_app_type(const char* appType);

    extern void bugsnag_set_app_release_stage(const char* stage);

    // Define individual user datums for subsequent notifications.
    // Setting a datum to NULL or "" causes it to take its default value.
    // The default user name is obtained from the OS if possible.
    // The default user_id and user_email are "".
    extern void bugsnag_set_user_name(const char* name);
    extern void bugsnag_set_user_id(const char* id);
    extern void bugsnag_set_user_email(const char* email);

    // Set the 'context' - this can be a string that explains the broad
    // operation ("job_create") or mode or phase the app is in.
    // Don't set this at all unless you are going to diligently update it
    // or it will just end up wrong and confusing.
    extern void bugsnag_set_context(const char* ctx);

    // Set the groupingHash - see the Bugsnag docs.  Pretty exotic.
    // Resets on notify.
    extern void bugsnag_set_grouping_hash(const char* grouping);

    // Define the entire metadata value for the next notification. Resets on notify.
    // You are responsible for formatting it as a valid JSON object { ... }
    // that follows the Bugsnag API specification:
    // https://docs.bugsnag.com/api/error-reporting/
    extern void bugsnag_set_metadata(const char* meta);

    // Notify Bugsnag of a single event
    extern int bugsnag_notify(BugsnagSeverity howbad, const char* errorClass, const char* message, const char* file, int lineNumber, const char* method);

#ifdef __cplusplus
}
#endif
#endif


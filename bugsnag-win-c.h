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
    // None of the other bugsnag functions will necessarily work before this is called.
    extern void bugsnag_init(const char* key);

    // Define the application version for subsequent notifications.
    // If app_version is set and an error is resolved in the dashboard, the
    // error will not unresolve until it is seen in a newer version.
    // version format is the usual a.b.c or a.b.c.d
    // If not set, or set to NULL, this defaults to reading the FILE_VERSION info
    // from the currently running executable.
    // If that can't be found, defaults to the empty string.
    extern void bugsnag_set_app_version(const char* version);

    // Set the kind of application. Typical values are "console", "service"
    // or "application".
    extern void bugsnag_set_app_type(const char* appType);

    // Set the application's 'release stage' (e.g. debug, beta, release, ...)
    // If not set, this defaults to:
    // 1. the value of the environment variable "BUGSNAG_STAGE" if present
    // 2. the $(Configuration) used to build the bugsnag-win-c module.
    // NOTE: the stage is always forced to all lower case (_strlwr)!
    extern void bugsnag_set_release_stage(const char* stage);

    // Specify a comma-separated list of release stages that should generate notifications.
    // If the current release stage is NOT in this list, bugsnag_notify does nothing.
    // e.g. "beta,release"
    // However, setting the stage_list to NULL or "" (the default) causes ALL
    // stages to generate notifications.
    extern void bugsnag_set_notify_stages(const char* stage_list);

    // Define current-user data for subsequent notifications.
    // Setting a datum to NULL or "" causes it to take its default value.
    // The default user name is obtained from the OS if possible.
    // The default user_id and user_email are "".
    extern void bugsnag_set_user_name(const char* name);
    extern void bugsnag_set_user_id(const char* id);
    extern void bugsnag_set_user_email(const char* email);

    // Set the 'context' - this can be a string that explains the broad
    // operation ("job_create") or mode or phase the app is in.
    // Don't set this at all unless you are pretty diligent about updating
    // it or it will just end up wrong and confusing.
    extern void bugsnag_set_context(const char* ctx);

    // Set the groupingHash - see the Bugsnag docs.  Pretty exotic.
    // Resets on notify.
    extern void bugsnag_set_grouping_hash(const char* grouping);

    // Define the entire metadata value for the next notification. Resets on notify.
    // You are responsible for formatting it as a valid JSON object { ... }
    // that follows the Bugsnag API specification:
    // https://docs.bugsnag.com/api/error-reporting/
    extern void bugsnag_set_metadata(const char* meta);

    // Notify Bugsnag of a single event.
    // howbad is BUGSNAG_ERROR, BUGSNAG_WARNING or BUGSNAG_INFO.
    // errorClass is the short key or code for this class of error e.g. "UnhandledStructuredException"
    // message is the human-readable description of the error or event
    // file & lineNumber are the name of the source code file and the line number in that file where the event occurred.
    // method is the name, if known, of the method or function where the event occurred.
    extern int bugsnag_notify(BugsnagSeverity howbad, const char* errorClass, const char* message, const char* file, int lineNumber, const char* method);

    // (macro that supplies the file, line & method arguments)
    // Notify Bugsnag of an event with severity, errorClass and message.
#define BUGSNAG_NOTIFY(sev, err, msg) do { bugsnag_notify(sev, err, msg, __FILE__, __LINE__, __func__); } while(0)

#ifdef __cplusplus
}
#endif
#endif


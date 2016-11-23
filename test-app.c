// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"

#define STRINGQ(x) #x
#define STRINGIZE(s) STRINGQ(s)

int main()
{
    bugsnag_init("9de2b53e68772c1efb56ae3a5153851b");
    bugsnag_set_user_email("spike.mclarty@machinemetrics.com");
    bugsnag_set_app_version("0.2.2.0");
    bugsnag_set_app_type("console");
    bugsnag_set_app_release_stage(STRINGIZE(CONFIGURATION));

    // fire a notification!
    bugsnag_set_metadata(
        "{\n"
        "      \"Machine\": { \"Name\": \"R-07-6\", \"Address\": \"10.2.250.2\", \"SHDR Port\": \"8002\" },"
        "      \"Adapter\": { \"Type\": \"fanuc-30i\", \"Version\": \"1.9.0.0\"  }"
        "}\n"
    );
    bugsnag_notify(
        BUGSNAG_ERROR,
        "ACCESS_DENIED",
        "illegal access to memory location",
        __FILE__, __LINE__,
        __func__
    );

    bugsnag_notify(
        BUGSNAG_WARNING,
        "Disk Space Low",
        "free space on the C: drive is below specified threshold",
        __FILE__, __LINE__,
        __func__
    );

    return 0;
}


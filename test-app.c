// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"

int main()
{
    bugsnag_set_api_key("9de2b53e68772c1efb56ae3a5153851b");
    bugsnag_set_user("Spike", "sjm", "spike.mclarty@machinemetrics.com");
    bugsnag_set_app_version("0.2.0.0");
    bugsnag_set_app_type("console");

    // fire a notification!
    bugsnag_notify(
        "ACCESS_DENIED",
        "illegal access to memory location",
        __FILE__, __LINE__,
        __func__
    );
    return 0;
}


// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"

#define STRINGQ(x) #x
#define STRINGIZE(s) STRINGQ(s)

int main(int argc, char* argv[])
{
    bugsnag_init("9de2b53e68772c1efb56ae3a5153851b");
    bugsnag_set_user_email("spike.mclarty@machinemetrics.com");
    bugsnag_set_app_type("console");
    bugsnag_set_metadata(
        "{\n"
        "      \"Machine\": { \"Name\": \"R-07-6\", \"Address\": \"10.2.250.2\", \"SHDR Port\": \"8002\" },"
        "      \"Adapter\": { \"Type\": \"fanuc-30i\", \"Version\": \"1.9.0.0\"  }"
        "}\n"
    );

    // fire a notification!
    BUGSNAG_NOTIFY(BUGSNAG_ERROR, "ACCESS_DENIED", "illegal access to memory location");

    return 0;
}


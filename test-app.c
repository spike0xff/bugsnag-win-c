// bugsnag-win-c.cpp : Defines the entry point for the console application.
//

#include "bugsnag-win-c.h"

int main()
{
    bugsnag_set_apiKey("9de2b53e68772c1efb56ae3a5153851b");
    // fire a notification!
    bugsnag_notify(
        "ACCESS_DENIED",
        "illegal access to memory location",
        "a really tremendously extra super maximally longer than you would ever (reasonably, under normal (not, like, weird) circumstances) expect any file path to be (under Windows (except those damn node_modules trees with npm)) filepath:" __FILE__, __LINE__,
        "main()"
    );
    return 0;
}


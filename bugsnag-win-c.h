#pragma once
#ifndef BUGSNAG_WIN_C_H
#define BUGSNAG_WIN_C_H
#ifdef __cplusplus
extern "C"
{
#endif

    // set the apiKey for your project on Bugsnag.
    extern void bugsnag_set_apiKey(const char* key);

#ifdef __cplusplus
}
#endif
#endif


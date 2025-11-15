#pragma once

#include <windows.h>
#include <dbghelp.h>

//#define MGR_DEBUG

void setup_handler();

void Traceback(CONTEXT* ctx);

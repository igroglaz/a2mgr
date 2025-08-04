#include <windows.h>
#include <string>

#include "utils.h"

const char* defaultHatURL = "hat.rom2.ru";
std::string hat;
char aHat[256];

std::string login;
const char* aLogin;
std::string password;
const char* aPassword;

void ParseLogin() {
    auto args = ParseCommandLine(GetCommandLineA());

    for (size_t i = 0; i+1 < args.size(); ++i) {
        if (args[i] == "-login") {
            login = args[i+1];
            aLogin = login.c_str();
        } else if (args[i] == "-password") {
            password = args[i+1];
            aPassword = password.c_str();
        }
    }
}

void __declspec(naked) HATENT_login_text() {
    ParseLogin();
    
    __asm
    {
        mov  eax, aLogin
        push eax
        mov  ecx, [ebp-0x124]
        add  ecx, 0x0C
        mov  edx, 0x005AB9E0
        call edx

        mov  eax, aPassword
        push eax
        mov  ecx, [ebp-0x124]
        add  ecx, 0x10
        mov  edx, 0x005AB9E0
        call edx

        //mov  edx, 0x0043F7ED //to password
        mov  edx, 0x0043F885
        jmp  edx
    }
}

void ParseHatAddress() {
    if (!hat.empty()) {
        return;
    }

    auto args = ParseCommandLine(GetCommandLineA());

    for (size_t i = 0; i+1 < args.size(); ++i) {
        if (args[i] == "-hat") {
            hat = args[i+1];
            log_format("Using hat address: '%s'\n", hat.c_str());
        }
    }

    if (hat.empty()) {
        hat = defaultHatURL;
    }

    if (hat.length() > sizeof(aHat)) {
        hat = hat.substr(0, sizeof(aHat)-1);
        log_format("Hat address cut to: '%s'. Have a shorter hat address :P\n", hat.c_str());
    }

    for (size_t i = 0; i < hat.length(); ++i) {
        aHat[i] = hat[i];
    }
    aHat[hat.length()] = '\0';
}

void __declspec(naked) HATENT_url_text() {
    ParseHatAddress();

    __asm
    {
        mov		eax, offset aHat
        push	eax
        mov		ecx, [ebp-0x124]
        add		ecx, 4
        mov		edx, 0x005AB9E0
        call	edx

        mov		edx, 0x0043F755
        jmp		edx

        // the following piece of code also replaces login and possibly password (if done to +0x10 addr)
        mov		eax, offset aLogin
        push	eax
        mov		ecx, [ebp-0x124]
        add		ecx, 0x0C
        mov		edx, 0x005AB9E0
        call	edx

        mov		edx, 0x0043F7ED
        jmp		edx
    }
}

void __declspec(naked) HATENT_url_connect() {
    ParseHatAddress();

    __asm
    {
        add		esp, 4
        mov     ecx, 0x0069C208
        mov		edx, 0x0051151C
        push	offset aHat // адрес хэта
        call    edx
        mov		dword ptr [ebp-0x224], eax
        mov		edx, 0x00494AF4
        jmp		edx
    }
}

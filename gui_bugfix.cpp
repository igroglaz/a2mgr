#include <windows.h>
#include <string>
#include "zxmgr.h"
#include "utils.h"
#include "config.h"

using namespace zxmgr;
using namespace std;

// Определение позиции курсора в тексте из координат клика на поле
// fix: более точное соответствие координат позиции курсора
// fix: исправлен курсор в запароленном поле
// fix: исправлено определение позиции в случае с нестандартным расположением текстового поля (X!=0)
unsigned long _stdcall GUI_textField_MeasurePosition(unsigned long x)
{
    unsigned long pthis;
    __asm mov pthis, ecx;

    unsigned long pos = 0;
    bool fpos = false;

    char* ctx = *(char**)(pthis+0x5C);

    string tx = "";

    if (*(unsigned long*)(pthis+0x7C)) // password field
    {
        for (size_t i = 0; i < strlen(ctx); i++)
            ctx[i] = '*';
    }

    for (size_t i = 0; i < strlen(ctx); i++)
    {
        tx += ctx[i];
        int wd = Font::MeasureTextWidth(*(unsigned long*)(pthis+0x60), tx.c_str());
        RECT* clrec = new RECT;
        GUI::GetClientRect(pthis, clrec);
        int innerpos = x-clrec->left;

        if (wd >= innerpos)
        {
            char thissym[2] = {ctx[i], 0};
            int wdx = (Font::MeasureTextWidth(*(unsigned long*)(pthis+0x60), thissym)-4)/2;

            if (wd-wdx >= innerpos)
                pos = i;
            else
                pos = i+1;

            fpos = true;

            break;
        }
    }

    if (!fpos) pos = strlen(ctx);

    return pos;
}

unsigned long OnGMEnter(unsigned long pthis, unsigned long arg0)
{
    unsigned long pthis_2 = 0;
    unsigned long pthis_3 = 0;
    unsigned long pthis_4 = 0;

    __asm
    {
        mov     edx, 0x00401870
        call    edx
        mov     [pthis_2], eax

        mov     ecx, [pthis_2]
        add     ecx, 0x468
        mov     [pthis_4], ecx

        
    }

    unsigned long p_id1 = *(unsigned long*)(pthis_4 + 0x08);
    unsigned long p_id2 = *(unsigned long*)(pthis_4 + 0x0C);

    if ((p_id2 & 0x3F000000) == 0x3F000000)
        return 1;

    return 0;
}

#pragma warning(push)
#pragma warning(disable : 4733)
void __declspec(naked) GUI_gmEnter()
{ // 44DB8D
    __asm
    {
        push    ebp
        mov     ebp, esp
        push    0xFFFFFFFF
        push    0x005C70DC
        mov     eax, fs:[0]
        push    eax
        mov     fs:[0], esp
        sub     esp, 0x34
        mov     [ebp-0x2C], ecx

        push    [ebp+0x08]
        push    ecx
        call    OnGMEnter
        add     esp, 8
        test    eax, eax
        jnz     ggme_ret1

        mov     edx, 0x0044DBAB
        jmp     edx

ggme_ret1:
        mov     eax, 1
        mov     ecx, [ebp-0x0C]
        mov     fs:[0], ecx
        mov     esp, ebp
        pop     ebp
        retn    0x0004
        
    }
}
#pragma warning(pop)

struct StatsInfo {
    uint32_t monster_kills, player_kills, frags;
    uint32_t deaths;
    uint32_t money;
    uint8_t body, reaction, mind, spirit;
    uint32_t spells, active_spell;
    uint32_t experience[5];
};

auto stats_info = (StatsInfo*)0x006995a8;

// That's a bit of a hack. The newly created character doesn't read the saved `a2c`
// file, so in `StatsBasedLevelSelection` we'll see the stats of the last selected
// character. To fix, when the client is rendering the character creation screen
// (specifically, the buttons to increase/decrease the base stats) remember that we're
// creating a new character.
bool creating_new_character = false;

int ParseServerNumber(const char* row) {
    // Row example:
    //  |[6:38] #1 Mind < 15|1.02|Road to Plagat|64x64|1|0|127.0.0.1:8001|
    // It's sent by hat, see `CLCMD_SendServerList`.
    // We're parsing the first part and extracting the level from it, so `#1` -> 1 in this example.

    if (row[0] != '|' || row[1] != '[') {
        log_format("stats_level: wrong row syntax (doesn't start with `|[`), failing open: '%s'\n", row);
        return 0;
    }

    const char* second_pipe = strchr(row+2, '|');
    if (!second_pipe) {
        log_format("stats_level: wrong row syntax (no second `|`), failing open: '%s'\n", row);
        return 0;
    }

    const char* hash = strstr(row, "] #");
    if (!hash) {
        log_format("stats_level: wrong row syntax (no `] #` found), failing open: '%s'\n", row);
        return 0;
    }

    if (hash > second_pipe) {
        log_format("stats_level: wrong row syntax (`] #` not found in first section), failing open: '%s'\n", row);
        return 0;
    }

    const char* level_ptr = hash + 3;
    if (!('1' <= *level_ptr && *level_ptr <= '9')) {
        log_format("stats_level: wrong row syntax (`#` not followed by a number), failing open: '%s'\n", level_ptr);
        return 0;
    }

    int server_number = 0;
    while ('0' <= *level_ptr && *level_ptr <= '9') {
        server_number = server_number * 10 + (*level_ptr - '0');
        level_ptr++;
    }

    return server_number;
}

bool __fastcall StatsBasedLevelSelection(uint8_t* obj, int row_number) {
    if (!stats_based_level_selection) {
        return true;
    }

    if (!obj) {
        log_format("stats_level: obj is null, failing open\n");
        return true;
    }

    uint8_t* subfield = *(uint8_t**)(obj + 0x68);
    const char* row = *(const char**)(subfield + row_number * 4);

    int server_number = ParseServerNumber(row);
    if (server_number == 0) {
        return true; // Fail open.
    }

    if (creating_new_character) {
        // New characters go to the first server.
        return server_number == 1;
    }

    switch (server_number) {
        case 1:
            return stats_info->mind < 15;
        case 2:
            return stats_info->mind >= 15 && stats_info->reaction < 20;
        case 3:
            return 20 <= stats_info->reaction && stats_info->reaction < 30;
        case 4:
            return 30 <= stats_info->reaction && stats_info->reaction < 40;
        case 5:
            return 40 <= stats_info->reaction && (stats_info->reaction < 50 || stats_info->mind < 50 || stats_info->spirit < 50);
        default:
            return stats_info->reaction >= 50 && stats_info->mind >= 50 && stats_info->spirit >= 50;
    }
}

// disable check player skills - is he allowed to enter the server (it was 26-50-90 brackets by default)
void __declspec(naked) GUI_softcoreEnter()
{
    __asm
    { // 44DD0F
        mov ecx, DWORD PTR [ebp-0x2c] // Current object, holds the row received from hat.
        mov edx, DWORD PTR [ebp+0x8] // Row number.
        call StatsBasedLevelSelection

        test al, al
        jz forbidden

        mov     [ebp-0x40], 1
        mov     edx, 0x0044DD55
        jmp     edx

    forbidden:
        mov     edx, 0x0044DD4E
        jmp     edx
    }
}

void NewCharacterScreen() {
    creating_new_character = true;
}

// Address: 0042b599
void __declspec(naked) new_character_screen() {
    NewCharacterScreen();

    __asm {
        mov DWORD PTR [ebp-0x4], 0xffffffff
        mov ebx, 0x0042b5a0
        jmp ebx
    }
}

void LoadCharacter() {
    creating_new_character = false;
}

// Address: 004f75f5
void __declspec(naked) load_character() {
    __asm {
        mov DWORD PTR[ecx], 0

        call LoadCharacter

        mov ecx, 0x004f75fb
        jmp ecx
    }
}

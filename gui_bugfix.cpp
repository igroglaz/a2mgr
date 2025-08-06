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

// disable check player skills - is he allowed to enter the server (it was 26-50-90 brackets by default)
void __declspec(naked) GUI_softcoreEnter()
{
    __asm
    { // 44DD0F
        mov     ecx, [ebp-0x18]
        add     ecx, 0x468

        mov     edx, [ebp-0x10]
        add     edx, 1

/*///////////////////////////////////
// we comment this out - to allow to enter all servers without
//  check on min-max skills (it's client UI block - greyish non-clickable line)

        cmp     [z_softcore], 0 // z_softcore in vanilla mode is 0 .. if equal: ZF = 1
        jnz     test_upper      // if not softcore - go test test

        cmp     [ecx+0x114], edx
        jg      test_failed

test_upper:

        cmp     [ecx+0x118], edx
        jl      test_failed

        cmp     [ebp-0x14], 0x10
        jge     test_failed
                                   //
///////////////////////////////////*/

        mov     [ebp-0x40], 1
        mov     edx, 0x0044DD55
        jmp     edx

        mov     edx, 0x0044DD4E
        jmp     edx
    }
}

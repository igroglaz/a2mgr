#include <windows.h>

#include "zxmgr.h"

int __fastcall InfoPanelLeftSpriteY(RECT* at) {
    const int sprite_height = 242;

    // Vanilla logic: draw the left part of the info panel at `y = at->top + 0x1e0 + 46`.
    // However, the right part (with all the text) is drawn at the bottom of the screen. Let's move the left part down.
    auto hwnd = zxmgr::GetHWND();
    RECT rect;
    if (GetWindowRect(hwnd, &rect)) {
        return rect.bottom - sprite_height;
    }

    // Can't get window size? Draw at the bottom of whatever rectangle we have. Shouldn't happen.
    // 242 is the sprite height.
    return at->bottom - sprite_height;
}

// Address: 0040a939
void __declspec(naked) info_panel_left_sprite() {
    __asm {
        lea ecx, DWORD PTR [ebp-0x48]
        call InfoPanelLeftSpriteY
        mov edx, eax
        mov eax, 0x0040a940
        jmp eax
    }
}

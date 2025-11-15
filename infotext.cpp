#include <windows.h>

int __fastcall InfoPanelLeftSpriteY(RECT* at) {
    // Vanilla logic: draw the left part of the info panel at `y = at->top + 0x1e0 + 46`.
    // However, the right part (with all the text) is drawn at the bottom of the screen. Let's move the left part down.
    return at->bottom - 242; // 242 is the sprite height.
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

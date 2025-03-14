#include <memory>
#include <unordered_map>
#include <windows.h>

#include "config.h"
#include "lib/serialize.hpp"
#include "stdint.h"
#include "utils.h"
#include "zxmgr.h"

struct Hotkey {
    enum class Kind: uint16_t {
        UNSET = 0,
        MAGIC = 1,
        ITEM = 2,
    };

    Kind kind;
    uint16_t to_id;
    uint32_t length;
    char* payload;
};

struct Item {
};

auto A2SetShift = (void (__thiscall *)(Hotkey* hotkey, Item* item))(0x0041fbc5);
auto A2IsItemMatches = (int (__thiscall *)(Hotkey* hotkey, Item* item))(0x0041fc31);
auto A2PickItem = (Item** (__thiscall *)(int, int))(0x00421870);
auto A2MagicHotkey = (void (__thiscall *)(void *obj, int event, int f_key_number, int ctrl_pressed))(0x004cf465);
auto A2ItemHotkey = (void (__thiscall *)(void *obj, int event, int f_key_number, int shift_pressed))(0x004ab46f);
auto A2SkipMagicPostprocess = (bool (__thiscall *)(void* obj))(0x0041cd38);
auto A2SerializeHotkey = (uint32_t (__thiscall *)(Hotkey* hotkey, char** buf))(0x0041fe6d);
auto A2DeserializeHotkey = (void (__thiscall *)(Hotkey* hotkey, char** buf))(0x0041ff09);

int8_t* key_handler_object = nullptr;
int8_t* window_object = nullptr;

auto ctrl = (volatile int*)0x62c9a0;
auto shift = (volatile int*)0x62c9a4;

struct BasicInfo {
    uint32_t id1;
    uint32_t id2;
    uint32_t hat_id;
    char nickname[32];
    uint8_t character_class;
};

auto basic_info = (volatile BasicInfo*)0x0069c058;

std::unordered_map<int, std::string> CreateLabels() {
    std::unordered_map<int, std::string> result{
        {VK_BACK, "BACK"},
        {VK_TAB, "TAB"},
        {VK_INSERT, "INS"},
        {VK_DELETE, "DEL"},
        {VK_HOME, "HOME"},
        {VK_END, "END"},
        {VK_OEM_PLUS, "="},
        {VK_OEM_MINUS, "-"},
        {VK_OEM_COMMA, ","},
        {VK_OEM_PERIOD, "."},
        {VK_OEM_1, ";"},
        {VK_OEM_2, "/"},
        {VK_OEM_3, "~~"},
        {VK_OEM_4, "["},
        {VK_OEM_5, "\\"},
        {VK_OEM_6, "]"},
        {VK_OEM_7, "\""},
        {VK_MULTIPLY, "NUM *"},
        {VK_ADD, "NUM +"},
        {VK_SUBTRACT, "NUM -"},
        {VK_DECIMAL, "NUM ."},
        {VK_DIVIDE, "NUM /"},
    };
    
    for (int key = VK_F1; key <= VK_F24; ++key) {
        result[key] = "F" + std::to_string(key - VK_F1 + 1);
    }

    return result;
}

std::unordered_map<int, std::string> labels = CreateLabels();

std::unordered_map<int, std::unique_ptr<Hotkey>> custom_hotkeys;
uint32_t custom_hotkeys_loaded_for = 0;

bool LoadHotkeys(BinaryStream& stream, std::unordered_map<int, std::unique_ptr<Hotkey>>& new_hotkeys) {
    auto signature = stream.ReadUInt32();
    if (signature != 0xCC795074) {
        log_format("[load_hotkey] incorrect signature: got 0x%x want 0x%x\n", signature, 0xCC795074);
        return false;
    }

    uint32_t total_hotkeys = stream.ReadUInt32();

    // 2560 bytes is likely an overkill, as shortcuts take `8 bytes + payload`, and payload is
    // 2 bytes for each magic property, and the game doesn't support more than 13 properties.
    // But original client does it this way (likely reusing the buffer for monster kills), so let it be.
    char buffer[2560];

    for (uint32_t i = 0; i < total_hotkeys; ++i) {
        if (stream.EndOfStream()) {
            log_format("[load_hotkey] end-of-stream at hotkey %u out of %u\n", i, total_hotkeys);
            return false;
        }

        int32_t key = stream.ReadInt32();
        uint32_t size = stream.ReadUInt32();
        stream.ReadData(buffer, size);

        new_hotkeys[key].reset(new Hotkey());
        char* buf = buffer;
        A2DeserializeHotkey(new_hotkeys[key].get(), &buf);
    }

    return true;
}

void LoadCustomHotkeys() {
    if (custom_hotkeys_loaded_for == basic_info->id1) {
        return;
    }

    BinaryStream stream;
    std::string filename = Format("hotkey.%u.a2key", basic_info->id1);

    if (hotkey_debug) {
        log_format("[load_hotkeys] Loading hotkeys from %s\n", filename.c_str());
    }

    if (!stream.LoadFromFile(filename)) {
        // File doesn't exist, so no hotkeys are defined.
        custom_hotkeys.clear();
        return;
    }

    std::unordered_map<int, std::unique_ptr<Hotkey>> new_hotkeys;
    if (!LoadHotkeys(stream, new_hotkeys)) {
        zxmgr::WriteChat("Failed to load hotkeys!");
        return;
    }

    custom_hotkeys = std::move(new_hotkeys);
    custom_hotkeys_loaded_for = basic_info->id1;
}

void SaveCustomHotkeys() {
    // File format is straightforward. Signature (4 bytes), number of hotkeys (4 bytes), list of hotkeys.
    // Each hotkey is: key (4 bytes), serialized hotkey size (4 bytes), serialized hotkey bytes.
    // Uses built-in client serialization of hotkeys.
    BinaryStream stream;

    stream.WriteUInt32(0xCC795074);
    stream.WriteUInt32(custom_hotkeys.size());

    char buffer[2560];

    for (auto it = custom_hotkeys.begin(); it != custom_hotkeys.end(); ++it) {
        stream.WriteUInt32(it->first);
        char* buf = buffer;
        uint32_t size = A2SerializeHotkey(it->second.get(), &buf);
        stream.WriteUInt32(size);
        stream.WriteData(buffer, size);
    }

    std::string filename = Format("hotkey.%u.a2key", basic_info->id1);

    if (hotkey_debug) {
        log_format("[save_hotkeys] Saving hotkeys to %s\n", filename.c_str());
    }

    if (!stream.SaveToFile(filename)) {
        zxmgr::WriteChat("Failed to save hotkeys!");
    }
}

bool IsTyping() {
    return window_object && *(int*)(window_object+0xc0) != 0;
}

bool AllowedWhileTyping(int key) {
    return VK_F1 <= key && key <= VK_F24 || key == VK_INSERT || key == VK_DELETE || key == VK_INSERT || key == VK_DELETE;
}

Hotkey* LookupHotkey(int key) {
    if (labels.count(key) == 0) {
        return nullptr;
    }

    if (window_object == nullptr) {
        return nullptr;
    }

    if (IsTyping() && !AllowedWhileTyping(key)) {
        return nullptr;
    }

    LoadCustomHotkeys();

    auto it = custom_hotkeys.find(key);
    if (it != custom_hotkeys.end() && it->second->kind != Hotkey::Kind::UNSET) {
        return it->second.get();
    }

    return nullptr;
}

bool HotkeysAvailable() {
    if (window_object == nullptr || key_handler_object == nullptr) {
        return false;
    }

    int game_state = *(int*)(window_object+0x41c);
    int some_object = *(int*)(key_handler_object+0x80);
    return game_state == 1 && some_object != 0;
}

bool IsHotkeyAction(int key) {
    if (labels.count(key) == 0) {
        return false;
    }

    if (!HotkeysAvailable()) {
        return false;
    }

    if (*shift || *ctrl) {
        return true;
    }
    
    auto hotkey = LookupHotkey(key);
    return hotkey && hotkey->kind != Hotkey::Kind::UNSET;
}

const char* Label(int key) {
    auto it = labels.find(key);
    if (it != labels.end()) {
        return it->second.c_str();
    }
    return "";
}

uint32_t __fastcall HotkeyHandler(int8_t* obj, int key, int8_t* window) {
    if (key_handler_object == nullptr) {
        key_handler_object = obj;
    } else if (key_handler_object != obj) {
        log_format("[hotkey_handler] unexpected different handler\n");
        key_handler_object = obj;
    }

    if (window_object == nullptr) {
        window_object = window;
    } else if (window_object != window) {
        log_format("[hotkey_handler] unexpected different window ptr\n");
        window_object = window;
    }

    if (hotkey_debug) {
        int game_state = *(int*)(window+0x41c);
        int some_object = *(int*)(obj+0x80);
        int is_typing = *(int*)(window+0xc0);
        log_format("[hotkey_handler] enter standard handler: obj=0x%x, key=0x%x;  win->0x41c=%d, this->0x80=%d, win->0xc0=%d\n", obj, key, game_state, some_object, is_typing);
    }

    if (IsHotkeyAction(key)) {
        if (hotkey_debug) {
            log_format("[hotkey_handler] jumping into the handler directly\n");
        }
        return 0x0040caff; // Jump right into F-key logic in this function.
    }

    return 0x0040c99d; // Resume normal function flow.
}

// Address: 0040c997
void __declspec(naked) hotkey_handler() {
    __asm {
        // Original instruction.
        mov DWORD PTR [ebp-0x4], eax

        mov ecx, [EBP + -0xc]
        mov edx, DWORD PTR [ebp+0x8]
        push eax
        call HotkeyHandler

        // Remember the address where we'll jump to, chosen in `HotkeyHandler`.
        mov ebx, eax

        // Replay original instruction.
        mov eax, DWORD PTR [ebp-0x4]

        // Jump to the chosen instruction.
        jmp ebx
    }
}

void __fastcall FKeyHandler(int8_t* obj, int key, int8_t* window) {
    if (hotkey_debug) {
        log_format("[f_key_handler] obj=0x%x, key=0x%x, window=0x%x\n", obj, key, window);
    }

    Hotkey* hotkey = LookupHotkey(key);
    if (!hotkey || hotkey->kind == Hotkey::Kind::UNSET) {
        if (!*shift && !*ctrl) {
            if (hotkey_debug) {
                log_format("no hotkey defined for 0x%x\n", key);
            }
            return;
        }
        
        custom_hotkeys[key].reset(new Hotkey());
        hotkey = custom_hotkeys[key].get();
    }

    if ((hotkey->kind != Hotkey::Kind::ITEM || *ctrl) && !*shift) {
        int8_t* mthis = *(int8_t**)(window + 0xec);
        A2MagicHotkey(mthis, 0x417, key - VK_F4, *ctrl);
    }

    if ((hotkey->kind != Hotkey::Kind::MAGIC || *shift) && !*ctrl) {
        int8_t* ithis = *(int8_t**)(window + 0xe8);
        A2ItemHotkey(ithis, 0x417, key - VK_F4, *shift);
    }

    if (hotkey->kind == Hotkey::Kind::MAGIC) {
        auto skipMagicPostProcess = A2SkipMagicPostprocess(obj);

        uint32_t spell_mask = 1 << hotkey->to_id;

        auto substruct = *(int8_t**)(window+0xd0);

        if (hotkey_debug) {
            log_format("[f_key_handler] post-spell: skipMagicPostProcess=%d, spell_mask=0x%x, [0x148]=0x%x, [0x150]=0x%x\n", skipMagicPostProcess, spell_mask, (*(uint32_t*)(substruct + 0x148)), (*(uint32_t*)(substruct + 0x150)));
        }

        if (!skipMagicPostProcess) {
            auto a2Dispatch = (void (__thiscall *)(void* obj, int n))(0x0041a74e);
            if ((*(uint32_t*)(substruct + 0x148)) & spell_mask) { // Mage's spellbook.
                a2Dispatch(obj, 9);
            } else if ((*(uint32_t*)(substruct + 0x150)) & spell_mask) { // Warriors spellbook (backed by scrolls).
                a2Dispatch(obj, 10);
            }
        }
    }
}

// Address: 0040caff
void __declspec(naked) f_key_handler() {
    __asm {
        mov ecx, DWORD PTR [ebp-0xc]
        mov edx, DWORD PTR [ebp+0x8]
        push DWORD PTR [ebp-0x4]
        call FKeyHandler
        
        // Jump to the end of the block. Original instruction is replayed in C++.
        mov ecx, 0x0040cc4b
        jmp ecx
    }
}

uint32_t __fastcall TopLevelKey(char* address, int key) {
    if (hotkey_debug) {
        log_format("[top_level_key] address=0x%x, key=0x%x\n", address, key);
    }

    if (*(int*)(address + 0x460) == 0 && IsHotkeyAction(key)) {
        if (hotkey_debug) {
            log_format("[top_level_key] jumping into the handler directly\n");
        }
        return 0x0048a399; // Block with all standard hotkeys (calls function `0040c989`).
    }

    return 0x0048a0e5; // Normal flow.
}

// Address: 0048a0de
void __declspec(naked) top_level_key() {
    __asm {
        // EAX is important, preserve it.
        push eax
        mov ecx, DWORD PTR[ebp-0x2c]
        mov edx, DWORD PTR[ebp+8]
        call TopLevelKey

        // Remember the address where we'll jump to, chosen in `TopLevelKey`.
        mov ebx, eax

        // Restore EAX.
        pop eax
        
        // Original instruction.
        cmp DWORD PTR[eax+0x460], 0x0

        // Jump to the chosen instruction.
        jmp ebx
    }
}

void __fastcall MarkItemHotkey(int8_t* obj, int y, int x1, int bag_item_position, int x2) {
    LoadCustomHotkeys();

    Item** item = A2PickItem(*(int*)(obj + 0x84), bag_item_position + **(int**)(obj + 0x90));

    for (auto it = custom_hotkeys.begin(); it != custom_hotkeys.end(); ++it) {
        Hotkey* hotkey = it->second.get();
        if (hotkey && hotkey->kind == Hotkey::Kind::ITEM) {
            if (A2IsItemMatches(hotkey, *item)) {
               zxmgr::Font::DrawText(FONT2, x2 + x1 + 35 + bag_item_position * 80, y + 8, Label(it->first), FONT_ALIGN_LEFT, FONT_COLOR_WHITE, 1);
            }
        }
    }
}

// Address: 004aac7c
void __declspec(naked) mark_item_hotkey() {
    __asm {
        push DWORD PTR [ebp-0x54]
        push DWORD PTR [ebp-0x44]
        push DWORD PTR [ebp-0x2c]
        mov edx, DWORD PTR [ebp-0x28]
        mov ecx, DWORD PTR [ebp-0x110]
        call MarkItemHotkey

        // Jump to the end of the block. Original instruction is replayed in C++.
        mov eax, 0x004aa92d
        jmp eax
    }
}

void __fastcall MarkSpellHotkey(int8_t* obj, int y, int x1, uint16_t spell_number, int x2) {
    LoadCustomHotkeys();

    for (auto it = custom_hotkeys.begin(); it != custom_hotkeys.end(); ++it) {
        Hotkey* hotkey = it->second.get();
        if (hotkey && hotkey->kind == Hotkey::Kind::MAGIC && hotkey->to_id == spell_number) {
            int offset = *(int*)(obj + 0x60) == spell_number ? 2 : 0;
            zxmgr::Font::DrawText(FONT2, x2 + x1 + 8 + offset + (spell_number % 0xc) * 0x26, y + 8 + offset + (spell_number / 0xc) * 0x26, Label(it->first), FONT_ALIGN_LEFT, FONT_COLOR_WHITE, 1);
        }
    }
}

// Address: 004cefba
void __declspec(naked) mark_spell_hotkey() {
    __asm {
        push DWORD PTR [ebp-0x70]
        push DWORD PTR [ebp-0x6c]
        push DWORD PTR [ebp-0x14]
        mov edx, DWORD PTR [ebp-0x10]
        mov ecx, DWORD PTR [ebp-0x8c]
        call MarkSpellHotkey

        // Jump to the end of the block. Original instruction is replayed in C++.
        mov eax, 0x004cf09a
        jmp eax
    }
}

Hotkey* __fastcall ItemHotkeyFind(char* item, int f_key_mangled) {
    int key = f_key_mangled / 12 + VK_F4;

    if (hotkey_debug) {
        log_format("[item_hotkey_find] f_key_mangled=%d (0x%x), item=0x%x\n", f_key_mangled, key, item);
    }

    return LookupHotkey(key);
}

// Address: 004ab61a
void __declspec(naked) item_hotkey_find() {
    __asm {
        call ItemHotkeyFind

        // Remember the hotkey address.
        mov ecx, eax

        // Jump to the function call (next instruction).
        mov eax, 0x004ab621
        jmp eax
    }
}

void __fastcall ItemHotkeySet(Item* item, int f_key_mangled) {
    int key = f_key_mangled / 12 + VK_F4;
    if (hotkey_debug) {
        log_format("[item_hotkey_set] f_key_mangled=%d (0x%x), item=0x%x\n", f_key_mangled, key, item);
    }

    Hotkey* hotkey = LookupHotkey(key);
    if (hotkey == nullptr) {
        custom_hotkeys[key].reset(new Hotkey());
        hotkey = custom_hotkeys[key].get();
    }

    if (A2IsItemMatches(hotkey, item)) {
        custom_hotkeys.erase(key);
    } else {
        A2SetShift(hotkey, item);

        if (hotkey_debug) {
            log_format("[item_hotkey_set]: %s is now [kind]=%d, [to_id]=%d, [length]=%d, [payload]=0x%x\n", Label(key), hotkey->kind, hotkey->to_id, hotkey->length, hotkey->payload);
        }
        
        std::vector<int> drop;

        for (auto it = custom_hotkeys.begin(); it != custom_hotkeys.end(); ++it) {
            if (it->first == key) {
                continue;
            }
            Hotkey* hotkey = it->second.get();
            if (hotkey && A2IsItemMatches(hotkey, item)) {
                drop.push_back(it->first);
            }
        }

        for (auto it = drop.begin(); it != drop.end(); ++it) {
            custom_hotkeys.erase(*it);
        }
    }

    SaveCustomHotkeys();
}

// Address: 004ab545
void __declspec(naked) item_hotkey_set() {
    __asm {
        pop ecx
        mov edx, eax
        call ItemHotkeySet

        // Jump to the end of the block. Original instruction is replayed in C++.
        mov eax, 0x004ab5b4
        jmp eax
    }
}

void __fastcall SpellHotkeyUse(int f_key_mangled, char* obj) {
    int key = f_key_mangled / 12 + VK_F4;

    if (hotkey_debug) {
        log_format("[spell_hotkey_use] f_key_mangled=%d (0x%x), obj=0x%x\n", f_key_mangled, key, obj);
    }

    Hotkey* hotkey = LookupHotkey(key);

    if (hotkey && hotkey->kind == Hotkey::Kind::MAGIC) {
        *(uint32_t*)(obj + 0x60) = static_cast<uint32_t>(hotkey->to_id);

        if (hotkey_debug) {
            log_format("[spell_hotkey_use] set current spell to %d\n", hotkey->to_id);
        }
    }
}

// Address: 004cf5df
void __declspec(naked) spell_hotkey_use() {
    __asm {
        mov ecx, eax
        mov edx, DWORD PTR [ebp-0x1c]
        call SpellHotkeyUse

        // Jump to the end of the block. Original instruction is replayed in C++.
        mov eax, 0x004cf607
        jmp eax
    }
}

void __fastcall SpellHotkeySet(short spell_number, int f_key_mangled) {
    int key = f_key_mangled / 12 + VK_F4;

    if (hotkey_debug) {
        log_format("[spell_hotkey_set] f_key_mangled=%d (0x%x), spell_num=%d\n", f_key_mangled, key, spell_number);
    }

    Hotkey* hotkey = LookupHotkey(key);
    if (hotkey == nullptr) {
        custom_hotkeys[key].reset(new Hotkey());
        hotkey = custom_hotkeys[key].get();
    }

    // Taken from Hotkey::SetSpellId_0041fba5.
    hotkey->kind = Hotkey::Kind::MAGIC;
    hotkey->to_id = spell_number;

    if (hotkey_debug) {
        log_format("[spell_hotkey_set]: %s is now [kind]=%d, [to_id]=%d, [length]=%d, [payload]=0x%x\n", Label(key), hotkey->kind, hotkey->to_id, hotkey->length, hotkey->payload);
    }

    std::vector<int> drop;
    
    for (auto it = custom_hotkeys.begin(); it != custom_hotkeys.end(); ++it) {
        if (it->first == key) {
            continue;
        }
        Hotkey* hotkey = it->second.get();
        if (hotkey && hotkey->kind == Hotkey::Kind::MAGIC && hotkey->to_id == spell_number) {
            drop.push_back(it->first);
        }
    }

    for (auto it = drop.begin(); it != drop.end(); ++it) {
        custom_hotkeys.erase(*it);
    }

    SaveCustomHotkeys();
}

// Address: 004cf4ee
void __declspec(naked) spell_hotkey_set_mouse() {
    __asm {
        mov ecx, edx
        mov edx, eax
        call SpellHotkeySet

        // Jump to the end of the block. Original instruction is replayed in C++.
        mov eax, 0x004cf5c6
        jmp eax
    }
}

// Address: 004cf562
void __declspec(naked) spell_hotkey_set_selected() {
    __asm {
        call SpellHotkeySet

        // Jump to the end of the block (same as `spell_hotkey_set_mouse`). Original instruction is replayed in C++.
        mov eax, 0x004cf5c6
        jmp eax
    }
}

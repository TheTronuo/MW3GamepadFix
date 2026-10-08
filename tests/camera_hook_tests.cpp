#include "mw3gf/game/build_profile.hpp"
#include "mw3gf/game/gameplay_profile.hpp"
#include <MinHook.h>
#include <Windows.h>
#include <array>
#include <cstring>
#include <iostream>
// Validate MinHook's instruction relocation for the verified native prologues
// in an isolated allocation. No game process, function call or Steam launch.
int main() {
    using namespace mw3gf::game;
    auto* page = static_cast<unsigned char*>(
        VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!page || MH_Initialize() != MH_OK)
        return 1;
    const std::array<std::array<std::uint8_t, 16>, 9> signatures{
        mouse_move_prologue,
        remote_move_prologue,
        {0x48, 0x8B, 0xC4, 0x48, 0x81, 0xEC, 0xA8, 0, 0, 0, 0xF3, 0x0F, 0x10, 0x2D, 0xCE, 0x7B},
        {0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x08, 0x57, 0x48, 0x81, 0xEC, 0xD0, 0, 0, 0, 0x48},
        find_asset_prologue,
        register_material_prologue,
        text_width_prologue,
        text_count_prologue,
        decoded_text_width_prologue};
    bool ok = true;
    for (std::size_t i = 0; i < signatures.size(); ++i) {
        void* target = page + i * 128;
        std::memset(target, 0x90, 64);
        std::memcpy(target, signatures[i].data(), signatures[i].size());
        // Complete the signature's last instruction with the actual bytes.
        // MouseMove ends inside cmp [rip+disp32], imm8 (disp high byte is zero).
        if (i == 0) {
            page[i * 128 + 16] = 0;
            page[i * 128 + 17] = 0;
        }
        // RemoteMove ends inside lea r8,[rip+disp32].
        else if (i == 1) {
            page[i * 128 + 16] = 0xf8;
            page[i * 128 + 17] = 0xff;
        } else if (i == 2) {
            page[i * 128 + 16] = 0x15;
            page[i * 128 + 17] = 0;
        } else if (i == 5) {
            page[i * 128 + 16] = 0xee;
            page[i * 128 + 17] = 0xff;
        } else if (i == 6) {
            page[i * 128 + 16] = 0x83;
            page[i * 128 + 17] = 0xec;
            page[i * 128 + 18] = 0x20;
        } else if (i == 7) {
            page[i * 128 + 16] = 0xc4;
            page[i * 128 + 17] = 0x08;
            page[i * 128 + 18] = 0xc3;
        } else if (i == 8) {
            page[i * 128 + 16] = 0x56;
        }
        void* trampoline{};
        const auto status = MH_CreateHook(target, page + 1024, &trampoline);
        if (status != MH_OK || !trampoline) {
            std::cerr << MH_StatusToString(status) << '\n';
            ok = false;
            break;
        }
        if (MH_EnableHook(target) != MH_OK || MH_DisableHook(target) != MH_OK ||
            std::memcmp(target, signatures[i].data(), signatures[i].size()) != 0 ||
            MH_RemoveHook(target) != MH_OK) {
            ok = false;
            break;
        }
    }
    MH_Uninitialize();
    VirtualFree(page, 0, MEM_RELEASE);
    if (ok)
        std::cout << "Camera, remote, cursor, text-render and renderer-teardown hooks relocate and restore\n";
    return ok ? 0 : 1;
}

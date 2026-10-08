#include "mw3gf/game/hook_set.hpp"
#include <Windows.h>
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace {
using Function = int (*)();
int replacement() {
    return 99;
}
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct ExecutablePage {
    unsigned char* data = static_cast<unsigned char*>(
        VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    ~ExecutablePage() {
        if (data)
            VirtualFree(data, 0, MEM_RELEASE);
    }
    Function function(std::size_t offset) {
        require(data != nullptr, "Cannot allocate executable test page");
        // mov eax, 7; ret, followed by executable padding for the trampoline.
        const std::array<unsigned char, 6> body{0xB8, 7, 0, 0, 0, 0xC3};
        std::memset(data + offset, 0x90, 64);
        std::memcpy(data + offset, body.data(), body.size());
        FlushInstructionCache(GetCurrentProcess(), data + offset, 64);
        return reinterpret_cast<Function>(data + offset);
    }
};
} // namespace

int main() {
    using namespace mw3gf::game;
    try {
        ExecutablePage page;
        const auto input = page.function(0);
        const auto renderer = page.function(128);
        Function original_input{}, original_renderer{};
        {
            HookSet hooks;
            hooks.add(HookId::mouse_move, reinterpret_cast<void*>(input), &replacement, original_input);
            hooks.add(HookId::render_text, reinterpret_cast<void*>(renderer), &replacement,
                      original_renderer);
            require(input() == 7 && renderer() == 7, "Hook creation enabled a detour prematurely");
            hooks.enable();
            require(input() == 99 && renderer() == 99, "Detours did not execute");
            require(original_input() == 7 && original_renderer() == 7,
                    "Trampolines changed original behavior");
            hooks.suspend_input();
            require(input() == 7 && renderer() == 99, "Stop did not retain the queued-render hook");
        }
        require(input() == 7 && renderer() == 7, "RAII did not restore enabled hooks");
        {
            HookSet hooks;
            hooks.add(HookId::mouse_move, reinterpret_cast<void*>(input), &replacement, original_input);
            bool rejected{};
            try {
                hooks.add(HookId::render_text, nullptr, &replacement, original_renderer);
            } catch (const std::runtime_error&) {
                rejected = true;
            }
            require(rejected, "Invalid hook target accepted");
        }
        // A failed partial installation must permit a fresh MinHook session.
        {
            HookSet hooks;
            hooks.add(HookId::mouse_move, reinterpret_cast<void*>(input), &replacement, original_input);
            bool rejected{};
            try {
                hooks.add(HookId::mouse_move, reinterpret_cast<void*>(renderer), &replacement,
                          original_renderer);
            } catch (const std::logic_error&) {
                rejected = true;
            }
            require(rejected, "Duplicate hook identity accepted");
            hooks.enable();
            require(input() == 99, "Installation after failure recovery did not work");
        }
        require(input() == 7 && renderer() == 7, "Recovery leaked an installed hook");
        std::cout << "Hook dispatch, trampolines, stop policy and partial-failure rollback verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

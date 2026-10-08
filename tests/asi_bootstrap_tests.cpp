#include <Windows.h>
#include <iostream>
int wmain(int argc, wchar_t** argv) {
    if (argc != 2)
        return 1;
    const HMODULE module = LoadLibraryW(argv[1]);
    if (!module) {
        std::cerr << "LoadLibrary failed: " << GetLastError() << '\n';
        return 1;
    }
    const auto init = reinterpret_cast<void (*)()>(GetProcAddress(module, "InitializeASI"));
    const bool exports =
        init && GetProcAddress(module, "MW3GamepadFix_Start") && GetProcAddress(module, "MW3GamepadFix_Stop");
    if (init) {
        init();
        init();
    } // An unrelated host must stay untouched; initialization is idempotent.
    FreeLibrary(module);
    if (!exports)
        return 1;
    std::cout << "ASI exports and non-game host guard verified\n";
    return 0;
}

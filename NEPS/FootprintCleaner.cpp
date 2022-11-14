#include "FootprintCleaner.hpp"

#include <memory>

bool FootprintCleaner::removeHeader() noexcept
{
    if (peHeader.data)
        return false;

    // Get DOS header
    const auto pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
    // Get NT header
    const auto pNTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<PBYTE>(pDosHeader) + pDosHeader->e_lfanew);

    // Check if NT header is correct
    if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
        return false;

    // Return value
    bool result = false;

    // Check if header exists and get the PE header's size
    if ((peHeader.size = pNTHeader->FileHeader.SizeOfOptionalHeader))
    {
        // Store VirtualProtect flags
        DWORD dwMemProtect;
        // Disable memory protection
        if (VirtualProtect(hModule, peHeader.size, PAGE_EXECUTE_READWRITE, &dwMemProtect))
        {
            // Allocate memory for PE header backup
            peHeader.data = std::make_unique<std::byte[]>(peHeader.size);
            // Backup PE header
            if (std::memcpy(peHeader.data.get(), hModule, peHeader.size) == hModule)
                // Erease PE header
                if (SecureZeroMemory(hModule, peHeader.size))
                    result = true;
            // Restore memory protection
            if (!VirtualProtect(hModule, peHeader.size, dwMemProtect, &dwMemProtect))
                result = false;
            // Release and delete the PE header if something went wrong
            if (!result)
            {
                peHeader.data.reset();
                peHeader.size = 0;
            }
        }
    }
    return result;
}

bool __thiscall FootprintCleaner::restoreHeader() noexcept
{
    if (!peHeader.data)
        return false;

    // Return value
    bool result = false;

    // Store VirtualProtect flags
    DWORD dwMemProtect = NULL;
    // Disable memory protection
    if (VirtualProtect(hModule, peHeader.size, PAGE_EXECUTE_READWRITE, &dwMemProtect))
    {
        // Restore PE header
        if (std::memcpy(hModule, peHeader.data.get(), peHeader.size) == hModule)
            result = true;
        // Restore memory protection
        if (!VirtualProtect(hModule, peHeader.size, dwMemProtect, &dwMemProtect))
            result = false;
        // Release and delete the PE header
        peHeader.data.reset();
        peHeader.size = 0;
    }

    return result;
}

bool FootprintCleaner::removeModule() noexcept
{
    if (unlinkedModule != nullptr)
        return false;

    // Get Process Environment Block
    const auto peb = reinterpret_cast<PPEB>(__readfsdword(FIELD_OFFSET(TEB, ProcessEnvironmentBlock)));

    // Cast PEB Loader Data as custom (PPEB_LDR_DATA_FULL) struct
    auto *ldr = reinterpret_cast<PPEB_LDR_DATA_FULL>(peb->Ldr);
    PLIST_ENTRY currentEntry = ldr->InLoadOrderModuleList.Flink;
    // Find an entry with the needed hModule
    while (currentEntry != &ldr->InLoadOrderModuleList && currentEntry != nullptr)
    {
        if (const auto current = CONTAINING_RECORD(currentEntry, LDR_DATA_TABLE_ENTRY_FULL, InLoadOrderLinks);
            current->DllBase == hModule)
        {
            // Backup current module state
            const UNLINKED_MODULE CurrentModule{
                hModule,
                current->InLoadOrderLinks.Blink->Flink,
                current->InMemoryOrderLinks.Blink->Flink,
                current->InInitializationOrderLinks.Blink->Flink,
                current
            };
            // Push backup to vector
            unlinkedModule = std::make_unique<UNLINKED_MODULE>(CurrentModule);

            // Unlink all
            unlinkLink(&current->InLoadOrderLinks);
            unlinkLink(&current->InMemoryOrderLinks);
            unlinkLink(&current->InInitializationOrderLinks);

            return true;
        }

        currentEntry = currentEntry->Flink;
    }

    return false;
}

bool __thiscall FootprintCleaner::restoreModule() noexcept
{
    if (unlinkedModule == nullptr)
        return false;

    // Relink all from backup
    relinkLink(unlinkedModule->RealInLoadOrderLinks, &unlinkedModule->LdrDataTableEntry->InLoadOrderLinks);
    relinkLink(unlinkedModule->RealInMemoryOrderLinks, &unlinkedModule->LdrDataTableEntry->InMemoryOrderLinks);
    relinkLink(unlinkedModule->RealInInitializationOrderLinks, &unlinkedModule->LdrDataTableEntry->InInitializationOrderLinks);
    // Delete backup
    unlinkedModule.reset();

    return true;
}

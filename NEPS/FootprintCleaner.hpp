#pragma once

#include <Windows.h>
#include <winternl.h>

#include <memory>

class FootprintCleaner
{
public:
    explicit FootprintCleaner(const HMODULE hModule) : hModule(hModule)
    {
        removeModule();
        removeHeader();
    }

    FootprintCleaner(const FootprintCleaner &) = delete;
    FootprintCleaner &operator=(const FootprintCleaner &) = delete;

    FootprintCleaner(FootprintCleaner &&) = delete;
    FootprintCleaner &operator=(FootprintCleaner &&) = delete;

    ~FootprintCleaner()
    {
        restoreHeader();
        restoreModule();
    }

private:
    // Why is there no header with these definitions?????
    typedef struct _PEB_LDR_DATA_FULL
    {
        ULONG Length;
        UCHAR Initialized;
        PVOID SsHandle;
        LIST_ENTRY InLoadOrderModuleList;
        LIST_ENTRY InMemoryOrderModuleList;
        LIST_ENTRY InInitializationOrderModuleList;
        PVOID EntryInProgress;
    } PEB_LDR_DATA_FULL, *PPEB_LDR_DATA_FULL;

    typedef struct _LDR_DATA_TABLE_ENTRY_FULL
    {
        LIST_ENTRY InLoadOrderLinks;
        LIST_ENTRY InMemoryOrderLinks;
        LIST_ENTRY InInitializationOrderLinks;
        PVOID DllBase;
        PVOID EntryPoint;
        ULONG SizeOfImage;
        UNICODE_STRING FullDllName;
        UNICODE_STRING BaseDllName;
        ULONG Flags;
        WORD LoadCount;
        WORD TlsIndex;
        union
        {
            LIST_ENTRY HashLinks;
            struct
            {
                PVOID SectionPointer;
                ULONG CheckSum;
            } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME;
        union
        {
            ULONG TimeDateStamp;
            PVOID LoadedImports;
        } DUMMYUNIONNAME2;
        _ACTIVATION_CONTEXT *EntryPointActivationContext;
        PVOID PatchInformation;
        LIST_ENTRY ForwarderLinks;
        LIST_ENTRY ServiceTagLinks;
        LIST_ENTRY StaticLinks;
    } LDR_DATA_TABLE_ENTRY_FULL, *PLDR_DATA_TABLE_ENTRY_FULL;

    typedef struct _UNLINKED_MODULE
    {
        HMODULE hModule;
        PLIST_ENTRY RealInLoadOrderLinks;
        PLIST_ENTRY RealInMemoryOrderLinks;
        PLIST_ENTRY RealInInitializationOrderLinks;
        PLDR_DATA_TABLE_ENTRY_FULL LdrDataTableEntry;
    } UNLINKED_MODULE, *PUNLINKED_MODULE;

    bool removeHeader() noexcept;
    bool restoreHeader() noexcept;
    bool removeModule() noexcept;
    bool restoreModule() noexcept;

    static constexpr auto unlinkLink(LIST_ENTRY *const source) noexcept
    {
        source->Flink->Blink = source->Blink;
        source->Blink->Flink = source->Flink;
    }

    static constexpr auto relinkLink(LIST_ENTRY *const source, LIST_ENTRY *const destenation) noexcept
    {
        destenation->Flink->Blink = source;
        destenation->Blink->Flink = source;
        source->Blink = destenation->Blink;
        source->Flink = destenation->Flink;
    }

    HMODULE hModule = nullptr;
    std::unique_ptr<UNLINKED_MODULE> unlinkedModule;
    struct __Data
    {
        std::unique_ptr<std::byte[]> data;
        DWORD size = NULL;
    } peHeader;
};

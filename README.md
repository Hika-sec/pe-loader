# PE-PACK BUILDER SYSTEM (v1.0.0)

A custom fileless PE packer that embeds a 64-bit Windows executable as a C++ byte array and executes it entirely in memory without writing to disk.

## Overview

This tool consists of two main components:

1. **Builder (`buidler.py`)** - A Python script that converts a target PE file into a C++ source file with the payload embedded as a hex byte array.
2. **Loader (`loaders/loader.cpp`)** - A Windows C++ template that performs manual PE mapping, relocation, import resolution, and execution.

## Features

- **Fileless execution** - The payload never touches disk
- **Manual PE parsing** - Full control over headers, sections, relocations, and imports
- **x64 only** - Designed for 64-bit Windows executables
- **Minimal dependencies** - Standard C++ with Windows API only
- **Compact output** - Optimized compilation flags for small binary size

## Project Structure

```
PE-LOADER/
├── buidler.py              # Python builder script
├── README.md               # This file
├── loaders/
│   └── loader.cpp          # PE loader template (contains /*Payload*/ placeholder)
└── stub/                   # Generated output directory
    └── *.cpp               # Generated C++ files with embedded payload
```

## Requirements

- Python 3.x
- GCC/G++ (MinGW or similar) for Windows target
- Windows 10/11 (x64) for execution

## Usage

### Step 1: Build the packed stub

```bash
python buidler.py -p <path_to_x64_exe> -o <output_name>
```

**Arguments:**
- `-p, --payload` - Path to the source x64 PE (.exe) file
- `-o, --output` - Name of the output C++ file (saved in `stub/` directory)

**Example:**
```bash
python buidler.py -p my_payload.exe -o packed_stub
```

This generates:
- `stub/packed_stub.cpp` - C++ source with embedded payload
- Prints a g++ compile command for the next step

### Step 2: Compile the stub

Use the g++ command printed by the builder (or manually):

```bash
g++ -Os -s -ffunction-sections -fdata-sections "-Wl,--gc-sections" "-Wl,--strip-all" -static-libgcc -static-libstdc++ stub/packed_stub.cpp -o packed_stub.exe
```

**Compilation flags explained:**
- `-Os` - Optimize for size
- `-s` - Strip symbols
- `-ffunction-sections` / `-fdata-sections` - Enable function/data section garbage collection
- `--gc-sections` - Remove unused sections
- `--strip-all` - Strip all debug/symbol information
- `-static-libgcc` / `-static-libstdc++` - Static linking of runtime libraries

## Technical Details

### PE Loader Process

The `MapAndExecutePE` function performs the following steps:

1. **Validation** - Checks DOS signature (`MZ`), NT signature, and x64 architecture
2. **Memory allocation** - Allocates `SizeOfImage` bytes with RWX permissions
3. **Section mapping** - Copies headers and all sections to their proper virtual addresses
4. **Base relocation** - Adjusts absolute addresses when the image loads at a different base than preferred
5. **Import resolution** - Loads referenced DLLs and resolves API function addresses
6. **PEB hijacking** - Updates `ImageBaseAddress` in the Process Environment Block
7. **Execution** - Creates a thread at the entry point and waits for completion

### Limitations

- Only supports x64 PE files
- No ASLR bypass (relocations handle non-preferred base addresses)
- No import table encryption or obfuscation
- Single-threaded execution model

## Security Considerations

This is a research/educational tool intended for:
- Malware analysis and research
- Security testing
- Understanding PE loading internals

**Warning:** Unauthorized use of this tool for malicious purposes is illegal. The authors are not responsible for misuse. Use only in controlled environments with proper authorization.

## License

This project is provided for educational and research purposes only.

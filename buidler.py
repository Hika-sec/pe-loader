import os
import argparse

GREEN = "\033[92m"
BLUE = "\033[94m"
CYAN = "\033[96m"
YELLOW = "\033[93m"
RED = "\033[91m"
RESET = "\033[0m"
BOLD = "\033[1m"

def print_banner():
    banner = f"""{CYAN}{BOLD}
       ██████▒ ▓█████        ███▄▄▄▄    ▄▄▄        ▄████▄   ███▄    █ 
      ▓██   ▒  ▓█   ▀        ███▀▀▀██▄ ▒████▄     ▒██▀ ▀█   ██ ▀█   █ 
      ▒████ ░  ▒███          ███   ███ ▒██  ▀█▄   ▒▓█    ▄  ██  ▀█  █ 
      ░▓██▄ ░  ▒▓█  ▄        ███   ███ ░██▄▄▄▄██  ▒▓▓▄ ▄██▒▓██  _█  █ 
      ░ ███ ░  ░▒████▒       ███   ███  ▓█   ▓██▒ ▒ ▓███▀ ░▒██▒ ░  ██▒
       ░ ▒░ ░  ░░ ▒░ ░        ▀█   █▀   ▒▒   ▓▒█░ ░ ░▒ ▒  ░░ ▒░   ▒ ▒ 
       ░ ░  ░   ░ ░  ░        ▒▓   █▒    ▒   ▒▒ ░   ░  ▒   ░ ░░   ░ ▒░
         ░        ░           ░▓   █░    ░   ▒    ░          ░   ░ ░  
                  ░  ░         ░   ░         ░  ░ ░                ░  
                                                  ░                   
    {RESET}{BLUE}==================================================================
    {GREEN}{BOLD}                  PE-PACK BUILDER SYSTEM (v1.0.0)                 
    {YELLOW}                        Custom Fileless Packer               
    {BLUE}=================================================================={RESET}
    """
    print(banner)

def main():
    print_banner()

    parser = argparse.ArgumentParser(description="Simple PE Joiner / Packer Builder")
    parser.add_argument("-p", "--payload", required=True, help="Path to the source x64 PE (.exe) file")
    parser.add_argument("-o", "--output", required=True, help="Name of the output C++ file (will be saved in stub/ directory)")
    args = parser.parse_args()

    template_filename = "loaders/loader.cpp"
    stub_dir = "stub"

    if not os.path.exists(template_filename):
        print(f"{RED}[-] Error: Source file '{template_filename}' not found in the loaders directory.{RESET}")
        return
    if not os.path.exists(args.payload):
        print(f"{RED}[-] Error: Payload file '{args.payload}' not found.{RESET}")
        return
    
    base_name = os.path.splitext(os.path.basename(args.output))[0]
    output_cpp_name = f"{base_name}.cpp"
    output_exe_name = f"{base_name}.exe"
    
    final_output_path = os.path.join(stub_dir, output_cpp_name)

    if not os.path.exists(stub_dir):
        os.makedirs(stub_dir)

    print(f"{BLUE}[+]{RESET} Reading template loader code from: {YELLOW}{template_filename}{RESET}")
    with open(template_filename, "r", encoding="utf-8") as f:
        loader_source = f.read()

    placeholder = "/*Payload*/"
    if placeholder not in loader_source:
        print(f"{RED}[-] Error: Target placeholder '{placeholder}' was not found in your loader.cpp.{RESET}")
        return

    print(f"{BLUE}[+]{RESET} Reading payload binary: {YELLOW}{args.payload}{RESET}")
    with open(args.payload, "rb") as f:
        payload_bytes = f.read()

    payload_size = len(payload_bytes)
    print(f"{BLUE}[+]{RESET} Total payload size: {GREEN}{payload_size} bytes{RESET}")
    print(f"{BLUE}[+]{RESET} Converting payload bytes to C++ HEX format...")

    hex_lines = []
    for i in range(0, payload_size, 12):
        chunk = payload_bytes[i:i+12]
        formatted_line = ", ".join([f"0x{b:02x}" for b in chunk])
        hex_lines.append("        " + formatted_line)

    array_content = "\n" + ",\n".join(hex_lines) + "\n    "

    print(f"{BLUE}[+]{RESET} Injecting HEX payload into the loader source...")
    final_source = loader_source.replace(placeholder, array_content)

    print(f"{BLUE}[+]{RESET} Writing final output file to: {GREEN}{final_output_path}{RESET}")
    with open(final_output_path, "w", encoding="utf-8") as f:
        f.write(final_source)

    print(f"\n{GREEN}{BOLD}[+] Build process finished successfully!{RESET}")
    
    compile_cmd = f'g++ -Os -s -ffunction-sections -fdata-sections "-Wl,--gc-sections" "-Wl,--strip-all" -static-libgcc -static-libstdc++ stub/{output_cpp_name} -o {output_exe_name}'
    
    print(f"{BLUE}[+]{RESET} To quickly compile your stub, run this command:")
    print(f"{YELLOW}{BOLD}{compile_cmd}{RESET}\n")

if __name__ == "__main__":
    main()

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ELF64 Structures
typedef struct {
    uint8_t  e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint64_t sh_flags;
    uint64_t sh_addr;
    uint64_t sh_offset;
    uint64_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint64_t sh_addralign;
    uint64_t sh_entsize;
} Elf64_Shdr;

typedef struct {
    uint32_t st_name;
    uint8_t  st_info;
    uint8_t  st_other;
    uint16_t st_shndx;
    uint64_t st_value;
    uint64_t st_size;
} Elf64_Sym;

// Assembly code for mystrcmp
const uint8_t text_section[] = {
    // mystrcmp code
    0x55,                               // push rbp
    0x48, 0x87, 0xf7,                   // xchg rsi, rdi
    0xc5, 0xfe, 0x6f, 0x06,             // vmovdqu ymm0, [rsi]
    0xc5, 0xfe, 0x6f, 0x0f,             // vmovdqu ymm1, [rdi]
    0xc5, 0xfd, 0x74, 0xd1,             // vpcmpeqb ymm2, ymm0, ymm1
    0xc5, 0xfd, 0x74, 0xd8,             // vpcmpeqb ymm3, ymm0, ymm0
    0xc5, 0xdd, 0x74, 0xe3,             // vpcmpeqb ymm4, ymm0, ymm3
    0xc5, 0xfd, 0xd7, 0xd2,             // vpmovmskb edx, ymm2
    0xc5, 0xfd, 0xd7, 0xc2,             // vpmovmskb eax, ymm2
    0x09, 0xd0,                         // or eax, edx
    0x39, 0xd0,                         // cmp eax, edx
    0x75, 0x04,                         // jne .not_equal
    0x48, 0x31, 0xc0,                   // xor rax, rax
    0x5d,                               // pop rbp
    0xc3,                               // ret

    // mystrlen code
    0x48, 0xc7, 0xc1, 0xff, 0xff, 0xff, 0xff, // mov rcx, -1
    0x30, 0xc0,                         // xor al, al
    0xf2, 0xae,                         // repne scasb
    0x48, 0xf7, 0xd1,                   // not rcx
    0x48, 0xff, 0xc9,                   // dec rcx
    0x48, 0x89, 0xc8,                   // mov rax, rcx
    0xc3                                // ret
};

// Section name string table
const char shstrtab[] = "\0.text\0.shstrtab\0.symtab\0.strtab";

// Symbol names string table
const char strtab[] = "\0mystrcmp\0mystrlen";

// Symbol table
const Elf64_Sym symtab[] = {
    // Null symbol (required)
    {0, 0, 0, 0, 0, 0},
    // mystrcmp (name offset 1 in strtab, function, global, in section 1 (.text))
    {1, 0x12, 0, 1, 0, 42},  // Size 42 bytes (just the mystrcmp part)
    // mystrlen (name offset 10 in strtab, function, global, in section 1 (.text))
    {10, 0x12, 0, 1, 43, 21}  // Starts at offset 43, size 21 bytes
};

int main() {
    FILE *out = fopen("mystrcmp.o", "wb");
    if (!out) {
        perror("fopen");
        return 1;
    }

    // Calculate sizes
    const size_t ehdr_size = sizeof(Elf64_Ehdr);
    const size_t shdr_size = sizeof(Elf64_Shdr);
    const size_t text_size = sizeof(text_section);
    const size_t shstrtab_size = sizeof(shstrtab);
    const size_t strtab_size = sizeof(strtab);
    const size_t symtab_size = sizeof(symtab);

    // ELF Header
    Elf64_Ehdr ehdr = {0};
    memcpy(ehdr.e_ident, "\x7F""ELF\2\1\1\0\0\0\0\0\0\0\0\0", 16);
    ehdr.e_type = 1;        // ET_REL
    ehdr.e_machine = 0x3E;  // EM_X86_64
    ehdr.e_version = 1;
    ehdr.e_shoff = ehdr_size;  // Section headers immediately follow ELF header
    ehdr.e_ehsize = ehdr_size;
    ehdr.e_shentsize = shdr_size;
    ehdr.e_shnum = 5;       // null + .text + .shstrtab + .symtab + .strtab
    ehdr.e_shstrndx = 2;    // .shstrtab is section 2

    // Write ELF header
    fwrite(&ehdr, 1, ehdr_size, out);

    // Calculate section offsets
    size_t current_offset = ehdr_size + 5 * shdr_size;  // After ELF header + 5 section headers

    // Section Headers
    Elf64_Shdr sections[5] = {0};

    // Null section header (required)
    sections[0].sh_type = 0;

    // .text section
    sections[1].sh_name = 1;      // "text" in shstrtab
    sections[1].sh_type = 1;      // SHT_PROGBITS
    sections[1].sh_flags = 6;     // SHF_ALLOC | SHF_EXECINSTR
    sections[1].sh_offset = current_offset;
    sections[1].sh_size = text_size;
    sections[1].sh_addralign = 16;
    current_offset += text_size;

    // .shstrtab section
    sections[2].sh_name = 7;      // "shstrtab" in shstrtab
    sections[2].sh_type = 3;      // SHT_STRTAB
    sections[2].sh_offset = current_offset;
    sections[2].sh_size = shstrtab_size;
    current_offset += shstrtab_size;

    // .symtab section
    sections[3].sh_name = 17;     // "symtab" in shstrtab
    sections[3].sh_type = 2;      // SHT_SYMTAB
    sections[3].sh_offset = current_offset;
    sections[3].sh_size = symtab_size;
    sections[3].sh_link = 4;      // Points to .strtab (section 4)
    sections[3].sh_info = 1;      // One local symbol (the null symbol)
    sections[3].sh_entsize = sizeof(Elf64_Sym);
    current_offset += symtab_size;

    // .strtab section
    sections[4].sh_name = 25;     // "strtab" in shstrtab
    sections[4].sh_type = 3;      // SHT_STRTAB
    sections[4].sh_offset = current_offset;
    sections[4].sh_size = strtab_size;
    current_offset += strtab_size;

    // Write section headers
    fwrite(sections, shdr_size, 5, out);

    // Write section data in order
    fwrite(text_section, 1, text_size, out);
    fwrite(shstrtab, 1, shstrtab_size, out);
    fwrite(symtab, 1, symtab_size, out);
    fwrite(strtab, 1, strtab_size, out);

    fclose(out);
    printf("Successfully generated fully-featured mystrcmp.o\n");

    // Verify with: readelf -h mystrcmp.o && readelf -S mystrcmp.o && objdump -d mystrcmp.o && readelf -s mystrcmp.o
    return 0;
}

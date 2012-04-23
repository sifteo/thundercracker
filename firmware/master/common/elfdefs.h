/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ELFDEFS_H
#define ELFDEFS_H

#include <stdint.h>

namespace Elf {

// http://www.sco.com/developers/gabi/latest/ch4.eheader.html
typedef uint32_t        Elf32_Addr;
typedef uint32_t        Elf32_Off;
typedef uint16_t        Elf32_Half;
typedef uint32_t        Elf32_Word;
typedef uint8_t         Elf32_Char;

enum FileType {
    ET_NONE     = 0,        // No file type
    ET_REL      = 1,        // Relocatablefile
    ET_EXEC     = 2,        // Executablefile
    ET_DYN      = 3,        // Shared object file
    ET_CORE     = 4,        // Corefile
    ET_LOOS     = 0xfe00,   // Operating system-specific
    ET_HIOS     = 0xfeff,   // Operating system-specific
    ET_LOPROC   = 0xff00,   // Processor-specific
    ET_HIPROC   = 0xffff    // Processor-specific
};

enum Machine {
    EM_NONE     = 0,    //No machine
    EM_ARM      = 40    // ARM 32-bit architecture (AARCH32)
    // lots more we don't care about for now...
};

enum Version {
    EV_NONE     = 0,    // Invalid version
    EV_CURRENT  = 1     // Current version
    // other future versions...
};

enum Class {
    ELFCLASSNONE    = 0,    // Invalid class
    ELFCLASS32      = 1,    // 32-bit objects
    ELFCLASS64      = 2     // 64-bit objects
};

enum ByteOrder {
    ELFDATANONE = 0,    // Invalid data encoding
    ELFDATA2LSB = 1,    // See below
    ELFDATA2MSB = 2     // See below
};

enum OsAbi {
    ELFOSABI_NONE   = 0,    // No extensions or unspecified
    ELFOSABI_GNU    = 3     // GNU
    // lots more ...
};

enum AccessFlags {
    PF_Exec     = 0x1,          // Execute
    PF_Write    = 0x2,          // Write
    PF_Read     = 0x4,          // Read
    PF_MASKOS   = 0x0ff00000,   // Unspecified
    PF_MASKPROC = 0xf0000000    // Unspecified
};

enum SegmentType {
    PT_NULL     = 0,
    PT_LOAD     = 1,
    PT_DYNAMIC  = 2,
    PT_INTERP   = 3,
    PT_NOTE     = 4,
    PT_SHLIB    = 5,
    PT_PHDR     = 6,
    PT_TLS      = 7,
    PT_LOOS     = 0x60000000,
    PT_HIOS     = 0x6fffffff,
    PT_LOPROC   = 0x70000000,
    PT_HIPROC   = 0x7fffffff
};

// indexes for the e_ident section
static const uint8_t EI_MAG0        = 0;    // File identification
static const uint8_t EI_MAG1        = 1;    // File identification
static const uint8_t EI_MAG2        = 2;    // File identification
static const uint8_t EI_MAG3        = 3;    // File identification
static const uint8_t EI_CLASS       = 4;    // File class
static const uint8_t EI_DATA        = 5;    // Data encoding
static const uint8_t EI_VERSION     = 6;    // File version
static const uint8_t EI_OSABI       = 7;    // Operating system/ABI identification
static const uint8_t EI_ABIVERSION  = 8;    // ABI version
static const uint8_t EI_PAD         = 9;    // Start of padding bytes
static const uint8_t EI_NIDENT      = 16;   // Size of e_ident[]

// EI_MAG
static const Elf32_Char Magic0 = 0x7f;
static const Elf32_Char Magic1 = 'E';
static const Elf32_Char Magic2 = 'L';
static const Elf32_Char Magic3 = 'F';

// special section indexes
static const Elf32_Half SHN_UNDEF       = 0;
static const Elf32_Half SHN_LORESERVE   = 0xff00;
static const Elf32_Half SHN_LOPROC      = 0xff00;
static const Elf32_Half SHN_HIPROC      = 0xff1f;
static const Elf32_Half SHN_LOOS        = 0xff20;
static const Elf32_Half SHN_HIOS        = 0xff3f;
static const Elf32_Half SHN_ABS         = 0xfff1;
static const Elf32_Half SHN_COMMON      = 0xfff2;
static const Elf32_Half SHN_XINDEX      = 0xffff;
static const Elf32_Half SHN_HIRESERVE   = 0xffff;

typedef struct {
    Elf32_Char e_ident[EI_NIDENT];   // file type identification data
    Elf32_Half e_type;      // adheres to FileType above
    Elf32_Half e_machine;   // adheres to Machine above
    Elf32_Word e_version;   // adheres to Version above
    Elf32_Addr e_entry;     // virtual address to which the system first transfers control, thus starting the process.
    Elf32_Off  e_phoff;     // program header table's file offset in bytes, or 0.
    Elf32_Off  e_shoff;     // section header table's file offset in bytes, or 0.
    Elf32_Word e_flags;     // processor-specific flags associated with the file
    Elf32_Half e_ehsize;    // ELF header's size in bytes
    Elf32_Half e_phentsize; // size in bytes of one entry in the file's program header table; all entries are the same size.
    Elf32_Half e_phnum;     // number of entries in the program header table, or 0 if no header table.
    Elf32_Half e_shentsize; // a section header's size in bytes
    Elf32_Half e_shnum;     // number of entries in the section header table
    Elf32_Half e_shstrndx;  // section header table index of the entry associated with the section name string table
} FileHeader;

typedef struct {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} ProgramHeader;

// Section header
typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off  sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} SectionHeader;

typedef struct {
    Elf32_Word    st_name;
    Elf32_Addr    st_value;
    Elf32_Word    st_size;
    unsigned char st_info;
    unsigned char st_other;
    Elf32_Half    st_shndx;
} Symbol;

// values for sh_type
static const Elf32_Word SHT_NULL            = 0;
static const Elf32_Word SHT_PROGBITS        = 1;
static const Elf32_Word SHT_SYMTAB          = 2;
static const Elf32_Word SHT_STRTAB          = 3;
static const Elf32_Word SHT_RELA            = 4;
static const Elf32_Word SHT_HASH            = 5;
static const Elf32_Word SHT_DYNAMIC         = 6;
static const Elf32_Word SHT_NOTE            = 7;
static const Elf32_Word SHT_NOBITS          = 8;
static const Elf32_Word SHT_REL             = 9;
static const Elf32_Word SHT_SHLIB           = 10;
static const Elf32_Word SHT_DYNSYM          = 11;
static const Elf32_Word SHT_INIT_ARRAY      = 14;
static const Elf32_Word SHT_FINI_ARRAY      = 15;
static const Elf32_Word SHT_PREINIT_ARRAY   = 16;
static const Elf32_Word SHT_GROUP           = 17;
static const Elf32_Word SHT_SYMTAB_SHNDX    = 18;
static const Elf32_Word SHT_LOOS            = 0x60000000;
static const Elf32_Word SHT_HIOS            = 0x6fffffff;
static const Elf32_Word SHT_LOPROC          = 0x70000000;
static const Elf32_Word SHT_HIPROC          = 0x7fffffff;
static const Elf32_Word SHT_LOUSER          = 0x80000000;
static const Elf32_Word SHT_HIUSER          = 0xffffffff;

// values for sh_flags
static const Elf32_Word SHF_WRITE               = 0x1;
static const Elf32_Word SHF_ALLOC               = 0x2;
static const Elf32_Word SHF_EXECINSTR           = 0x4;
static const Elf32_Word SHF_MERGE               = 0x10;
static const Elf32_Word SHF_STRINGS             = 0x20;
static const Elf32_Word SHF_INFO_LINK           = 0x40;
static const Elf32_Word SHF_LINK_ORDER          = 0x80;
static const Elf32_Word SHF_OS_NONCONFORMING    = 0x100;
static const Elf32_Word SHF_GROUP               = 0x200;
static const Elf32_Word SHF_TLS                 = 0x400;
static const Elf32_Word SHF_MASKOS              = 0x0ff00000;
static const Elf32_Word SHF_MASKPROC            = 0xf0000000;

// values for low nybble of st_info
static const unsigned char STT_NOTYPE       = 0;
static const unsigned char STT_OBJECT       = 1;
static const unsigned char STT_FUNC         = 2;
static const unsigned char STT_SECTION      = 3;
static const unsigned char STT_FILE         = 4;


}   // namespace Elf

#endif // ELFDEFS_H

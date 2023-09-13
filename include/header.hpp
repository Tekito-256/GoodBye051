#pragma once

#include "types.hpp"

// From https://gitlab.com/thepixellizeross/3gxtool

#define _3GX_MAGIC (0x3230303024584733) /* "3GX$0002" */
#define _3GX_MAGIC_OLD (0x3130303024584733) /* "3GX$0001" */

struct _3gx_Infos
{
    u32             authorLen{0};
    u32             authorMsg{0};
    u32             titleLen{0};
    u32             titleMsg{0};
    u32             summaryLen{0};
    u32             summaryMsg{0};
    u32             descriptionLen{0};
    u32             descriptionMsg{0};
    union {
        u32         flags{0};
        struct {
            u32     embeddedExeDecryptFunc : 1;
            u32     embeddedSwapEncDecFunc : 1;
            u32     memoryRegionSize : 2;
            u32     compatibility : 2;
            u32     unused : 26;
        };
    };
    u32             exeDecChecksum{0};
    u32             builtInDecExeArgs[4]{0};
    u32             builtInSwapEncDecArgs[4]{0};
} PACKED;

struct _3gx_Targets
{
    u32             count{0};
    u32             titles{0};
} PACKED;

struct _3gx_Executable
{
    u32             codeOffset{0};
    u32             rodataOffset{0};
    u32             dataOffset{0};
    u32             codeSize{0};
    u32             rodataSize{0};
    u32             dataSize{0};
    u32             bssSize{0};
    u32             exeDecOffset{0}; // NOP terminated
    u32             swapEncOffset{0}; // NOP terminated
    u32             swapDecOffset{0}; // NOP terminated
} PACKED;

struct _3gx_Header
{
    u64             magic{0};
    u32             version{0};
    u32             reserved{0};
    _3gx_Infos      infos{};
    _3gx_Executable executable{};
    _3gx_Targets    targets{};
    u32             syms[3];
} PACKED;

struct _3gx_Infos_Old
{
    u32             authorLen{0};
    u32             authorMsg{0};
    u32             titleLen{0};
    u32             titleMsg{0};
    u32             summaryLen{0};
    u32             summaryMsg{0};
    u32             descriptionLen{0};
    u32             descriptionMsg{0};
} PACKED;

struct _3gx_Targets_Old
{
    u32             count{0};
    u32             titles{0};
} PACKED;

struct _3gx_Header_Old
{
    u64             magic{0};
    u32             version{0};
    u32             codeSize{0};
    u32             code{0};
    _3gx_Infos_Old      infos{};
    _3gx_Targets_Old    targets{};
} PACKED;

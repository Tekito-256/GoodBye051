#include "Converter.hpp"

#include <iostream>

Converter::Converter(const std::string &inPath)
{
    _in.open(inPath, std::ios::binary);
}

Converter::~Converter()
{
    if(_in.is_open())
    {
        _in.close();
    }
}

void Converter::ConvertHeader(_3gx_Header &newHeader, const _3gx_Header_Old &oldHeader)
{
    _3gx_Infos &infos = newHeader.infos;
    _3gx_Executable &executable = newHeader.executable;
    _3gx_Targets &targets = newHeader.targets;

    memset(&newHeader, 0, sizeof(_3gx_Header));

    newHeader.magic = _3GX_MAGIC;
    newHeader.version = oldHeader.version;
    newHeader.reserved = 0;

    // Infos len
    infos.authorLen = oldHeader.infos.authorLen;
    infos.titleLen = oldHeader.infos.titleLen;
    infos.summaryLen = oldHeader.infos.summaryLen;
    infos.descriptionLen = oldHeader.infos.descriptionLen;

    infos.embeddedExeDecryptFunc = 1;
    infos.embeddedSwapEncDecFunc = 1;
    infos.compatibility = 0;
    infos.exeDecChecksum = 0;

    // Targets
    targets.count = oldHeader.targets.count;

    // Executable
    executable.codeSize = oldHeader.codeSize;
}

static const u32 plgLdrFetchEvent[] =
{
    0xE59F3018,
    0xE5932008,
    0xE1920F9F,
    0xE1823F90,
    0xE6EF3073,
    0xE3530000,
    0x1AFFFFFA,
    0xE12FFF1E,
    0x00000000 // Data offset
};

static const u32 plgLdrReply[] =
{
    0xE92D40F7,
    0xE3A03000,
    0xE1A04000,
    0xE59F50A0,
    0xE595100C,
    0xE1912F9F,
    0xE1812F93,
    0xE6EF2072,
    0xE3520000,
    0x1AFFFFFA,
    0xE3540002,
    0xCA000007,
    0xE5951008,
    0xE1913F9F,
    0xE1813F92,
    0xE6EF3073,
    0xE3530000,
    0x1AFFFFFA,
    0xE28DD00C,
    0xE8BD80F0,
    0xE3A06000,
    0xE3A07000,
    0xE3A03001,
    0xE1CD60F0,
    0xE5950004,
    0x00000000, // svcArbitrateAddress
    0xE3540003,
    0x1A00000D,
    0xE3E02000,
    0xE5951008,
    0xE1913F9F,
    0xE1813F92,
    0xE6EF3073,
    0xE3530000,
    0x1AFFFFFA,
    0xE3A06000,
    0xE3A07000,
    0xE3A02001,
    0xE1CD60F0,
    0xE5950004,
    0x00000000, // svcArbitrateAddress
    0xEAFFFFE7,
    0xE3540004,
    0x1AFFFFE5,
    0xEF000009, // svcExitThread
    0x00000000, // Data offset
};

static const u32 plgLdrInit[] =
{
    0xE92D4070,
    0xEE1D5F70,
    0xE59F403C,
    0xE28F103C,
    0xE1A00004,
    0x00000000, // svcConnectToPort
    0xE3A03809,
    0xE5940000,
    0xE5853080,
    0xEF000032,
    0xE3500000,
    0xA595308C,
    0xA5843004,
    0xE3A03407,
    0xE5932018,
    0xE5842008,
    0xE593301C,
    0xE584300C,
    0xE8BD8070,
    0x00000000, // Data offset
    0x3A676C70,
    0x0072646C
};

static const u32 initEx[] =
{
    0xE52DE004, // push {lr}
    0x00000000, // Kernel::Initialize
    0x00000000, // plgLdrInit
    0x00000000, // ConvertHeader
    0xE49DF004, // pop {pc}
};

static const u32 convertHeader[] =
{
    0xE3A03407,
    0xE5932014,
    0xE5832018,
    0xE12FFF1E
};

// From https://gitlab.com/thepixellizeross/ctrpluginframework/-/blame/develop/Library/source/CTRPluginFrameworkImpl/System/HookManager.cpp#L61
u32 CreateBranchARM(u32 from, u32 to, bool link)
{
    u32 instrBase = (link ? 0xEB000000 : 0xEA000000);
    u32 off = (u32)(to - (from + 8));

    return instrBase | ((off >> 2) & 0xFFFFFF);
}

// From https://github.com/citra-emu/citra/blob/f0a582b218f83f19d98c8df0e6130a32f022368b/src/core/arm/disassembler/arm_disasm.cpp#L314
u32 GetBranchAddr(u32 addr, u32 value)
{
    if(((value >> 26) & 0x3) != 2)
        return 0;
    
    u8 bit25 = (value >> 25) & 0x1;

    if (bit25 == 0) return 0;

    u32 offset = value & 0xffffff;

    if ((offset >> 23) & 1)
        offset |= 0xff000000;

    offset <<= 2;
    offset += 8;
        
    return addr + offset;
}

u32 OffsetToVA(const _3gx_Header &header, u32 offset)
{
    u32 codeBaseVa = 0x7000100;
    u32 codeStartOff = header.executable.codeOffset;
    int diff = offset - codeStartOff;

    if (diff < 0)
        return 0;

    return codeBaseVa + diff;
}

void Converter::Write(std::fstream &out, _3gx_Header &header, const _3gx_Header_Old &oldHeader)
{
    u8 *buffer;
    char infoBuf[1024];
    _3gx_Executable &executable = header.executable;
    _3gx_Infos &infos = header.infos;
    _3gx_Targets &targets = header.targets;

    out.write((const char *)&header, sizeof(_3gx_Header));

    // Write Infos
    infos.titleMsg = out.tellp();
    _in.seekg(oldHeader.infos.titleMsg);
    _in.read((char *)infoBuf, oldHeader.infos.titleLen);
    out.write(infoBuf, oldHeader.infos.titleLen);

    infos.authorMsg = out.tellp();
    _in.seekg(oldHeader.infos.authorMsg);
    _in.read((char *)infoBuf, oldHeader.infos.authorLen);
    out.write(infoBuf, oldHeader.infos.authorLen);

    infos.summaryMsg = out.tellp();
    _in.seekg(oldHeader.infos.summaryMsg);
    _in.read((char *)infoBuf, oldHeader.infos.summaryLen);
    out.write(infoBuf, oldHeader.infos.summaryLen);

    infos.descriptionMsg = out.tellp();
    _in.seekg(oldHeader.infos.descriptionMsg);
    _in.read((char *)infoBuf, oldHeader.infos.descriptionLen);
    out.write(infoBuf, oldHeader.infos.descriptionLen);

    // Write Targets
    targets.titles = out.tellp();
    _in.seekg(oldHeader.targets.titles);
    _in.read((char *)infoBuf, oldHeader.targets.count * sizeof(u32));
    out.write(infoBuf, oldHeader.targets.count * sizeof(u32));

    // Write embedded function
    u32 nop[] = { 0xE3A00000, 0xE320F000 };
    executable.exeDecOffset = out.tellp();
    executable.swapEncOffset = out.tellp();
    executable.swapDecOffset = out.tellp();
    out.write((const char *)nop, sizeof(nop));

    // Create padding
    executable.codeOffset = static_cast<u32>(out.tellp());
    u32 padding = 16 - (executable.codeOffset & 0xF);
    char zeroes[16] = {0};
    out.write(zeroes, padding);
    executable.codeOffset += padding;

    // Write code
    buffer = new u8[oldHeader.codeSize];

    _in.seekg(oldHeader.code);
    _in.read((char *)buffer, oldHeader.codeSize);
    out.write((const char *)buffer, oldHeader.codeSize);

    // Write ext data
    static u32 datas[4]; // plgLdrHandle, plgLdrArbiter, plgEvent, plgReply
    plgLdrDataOffset = out.tellp();
    out.write((const char *)datas, sizeof(datas));
    executable.codeSize += sizeof(datas);

    // Write builtin function
    plgLdrFetchEventOffset = out.tellp();
    out.write((const char *)plgLdrFetchEvent, sizeof(plgLdrFetchEvent));
    executable.codeSize += sizeof(plgLdrFetchEvent);

    plgLdrReplyOffset = out.tellp();
    out.write((const char *)plgLdrReply, sizeof(plgLdrReply));
    executable.codeSize += sizeof(plgLdrReply);

    plgLdrInitOffset = out.tellp();
    out.write((const char *)plgLdrInit, sizeof(plgLdrInit));
    executable.codeSize += sizeof(plgLdrInit);

    initExOffset = out.tellp();
    out.write((const char *)initEx, sizeof(initEx));
    executable.codeSize += sizeof(initEx);

    convertHeaderOffset = out.tellp();
    out.write((const char *)convertHeader, sizeof(convertHeader));
    executable.codeSize += sizeof(convertHeader);

    // Rewrite header
    out.seekp(0);
    out.write((const char *)&header, sizeof(_3gx_Header));

    delete[] buffer;
}

int Converter::Search(const PatternMask &pattern)
{
    if(pattern.size() == 0 || _buffer == nullptr || _bufferSize == 0)
        return -1;
    
    for(u32 i = 0; i < _bufferSize - pattern.size() * sizeof(u32); i++)
    {
        u32 *ptr = (u32 *)(_buffer + i);
        bool match = true;

        for(u32 j = 0; j < pattern.size(); j++)
        {
            if((ptr[j] & ~pattern[j].second) != (pattern[j].first & ~pattern[j].second))
            {
                match = false;
                break;
            }
        }
        if(match)
        {
            return (int)i;
        }
    }

    return -1;
}

bool Converter::Patch(const _3gx_Header &header)
{
    static const PatternMask keepThreadMainLoopPattern =
    {
        {0xE59D3014, 0},
        {0xE1CD80F0, 0},
        {0xE58D3038, 0},
        {0xE59D3018, 0},
        {0xE3A02002, 0},
        {0xE58D303C, 0},
        {0xE1A01004, 0},
        {0xE3A03000, 0},
        {0xE28D0020, 0},
        {0xE58D5020, 0},
        {0xEB000000, 0xFFFFFF},
        {0xE59D3020, 0},
        {0xE3530000, 0},
        {0x1A00000B, 0},
        {0xEB000000, 0xFFFFFF},
        {0xEAFFFFEF, 0},
    };

    int loopOffset = Search(keepThreadMainLoopPattern);

    if(loopOffset == -1)
    {
        return false;
    }

    static const PatternMask svcArbitrateAddressPattern =
    {
        {0xE92D0030, 0},
        {0xE59D4008, 0},
        {0xE59D500C, 0},
        {0xEF000022, 0},
        {0xE8BD0030, 0},
        {0xE12FFF1E, 0},
    };

    int svcArbitrateAddressOffset = Search(svcArbitrateAddressPattern);

    if(svcArbitrateAddressOffset == -1)
    {
        return false;
    }

    static const PatternMask svcConnectToPortPattern =
    {
        {0xE52D0004, 0},
        {0xEF00002D, 0},
        {0xE49D3004, 0},
        {0xE5831000, 0},
        {0xE12FFF1E, 0},
    };

    int svcConnectToPortOffset = Search(svcConnectToPortPattern);

    if(svcConnectToPortOffset == -1)
    {
        return false;
    }

    // Config plgLdrInit
    u32 *plgLdrInitPtr = (u32 *)&_buffer[plgLdrInitOffset];
    plgLdrInitPtr[5] = CreateBranchARM(OffsetToVA(header, plgLdrInitOffset + 5 * sizeof(u32)), OffsetToVA(header, svcConnectToPortOffset), true);
    plgLdrInitPtr[19] = OffsetToVA(header, plgLdrDataOffset);

    // Config plgLdrFetchEvent
    u32 *plgLdrFetchEventPtr = (u32 *)&_buffer[plgLdrFetchEventOffset];
    plgLdrFetchEventPtr[8] = OffsetToVA(header, plgLdrDataOffset);

    // Config plgLdrReply
    u32 *plgLdrReplyPtr = (u32 *)&_buffer[plgLdrReplyOffset];
    plgLdrReplyPtr[25] = CreateBranchARM(OffsetToVA(header, plgLdrReplyOffset + 25 * sizeof(u32)), OffsetToVA(header, svcArbitrateAddressOffset), true);
    plgLdrReplyPtr[40] = CreateBranchARM(OffsetToVA(header, plgLdrReplyOffset + 40 * sizeof(u32)), OffsetToVA(header, svcArbitrateAddressOffset), true);
    plgLdrReplyPtr[45] = OffsetToVA(header, plgLdrDataOffset);

    u32 *ptr = (u32 *)&_buffer[loopOffset];

    // Init hook
    u32 kernelInitOp = ptr[-181];
    u32 kernelInitAddr = GetBranchAddr(OffsetToVA(header, (u64)&ptr[-181] - (u64)_buffer), kernelInitOp);
    ptr[-181] = CreateBranchARM(OffsetToVA(header, (u64)&ptr[-181] - (u64)_buffer), OffsetToVA(header, initExOffset), true); // bl initEx

    // Config initEx
    u32 *initExPtr = (u32 *)&_buffer[initExOffset];
    initExPtr[1] = CreateBranchARM(OffsetToVA(header, initExOffset + 1 * sizeof(u32)), kernelInitAddr, true);
    initExPtr[2] = CreateBranchARM(OffsetToVA(header, initExOffset + 2 * sizeof(u32)), OffsetToVA(header, plgLdrInitOffset), true);
    initExPtr[3] = CreateBranchARM(OffsetToVA(header, initExOffset + 3 * sizeof(u32)), OffsetToVA(header, convertHeaderOffset), true);

    *ptr++ = 0xE59D0014; // ldr r0, =memLayoutChanged
    *ptr++ = 0xE59F20AC; // ldr r2, =100000000
    *ptr++ = 0xE3A03000; // mov r3, #0
    *ptr++ = 0xEF000024; // svcWaitSynchronization
    *ptr++ = 0xE59F50A4; // ldr r5, =0x9401BFE
    *ptr++ = 0xE1500005; // cmp r5, r0
    *ptr++ = 0x1A000006; // bne MemoryLayoutChangedHandler
    *ptr++ = CreateBranchARM(OffsetToVA(header, (u64)ptr - (u64)_buffer), OffsetToVA(header, plgLdrFetchEventOffset), true); // bl PLGLDR__Fetch
    *ptr++ = 0xE3500004; // cmp r0, #4
    *ptr++ = 0x0A00000F; // beq Exit
    *ptr++ = CreateBranchARM(OffsetToVA(header, (u64)ptr - (u64)_buffer), OffsetToVA(header, plgLdrReplyOffset), true); // bl PLGLDR__Reply
    *ptr++ = 0xEAFFFFF3; // b start
    *ptr++ = 0xE320F000;
    *ptr++ = 0xE320F000;
    ptr++; // bl UpdateMemRegions
    ptr++; // b loopBegin

    u32 *exitProcessPtr = &((u32 *)&_buffer[loopOffset])[44];
    *exitProcessPtr++ = 0xE3A00004; // mov r0, #4
    *exitProcessPtr++ = CreateBranchARM(OffsetToVA(header, (u64)exitProcessPtr - (u64)_buffer), OffsetToVA(header, plgLdrReplyOffset), true); // bl PLGLDR__Reply
    *exitProcessPtr++ = 0x05F5E100;
    *exitProcessPtr++ = 0x09401BFE;

    return true;
}

Converter::ConvertResult Converter::Convert(const std::string &outPath)
{
    std::fstream out(outPath, std::ios::binary | std::ios::out | std::ios::in | std::ios::trunc);
    _3gx_Header_Old oldHeader;
    _3gx_Header newHeader;

    if(!_in.is_open())
    {
        return FileNotFound;
    }

    _in.read((char *)&oldHeader, sizeof(_3gx_Header_Old));

    if(oldHeader.magic != _3GX_MAGIC_OLD)
    {
        return InvalidFormat;
    }

    ConvertHeader(newHeader, oldHeader);
    Write(out, newHeader, oldHeader);

    out.flush();

    // Get size
    out.seekg(0, std::ios::end);
    _bufferSize = out.tellg();
    _buffer = new char[_bufferSize];

    // Read file
    out.seekg(0);
    out.read(_buffer, _bufferSize);

    if(!Patch(newHeader))
    {
        delete[] _buffer;
        return PatchFailed;
    }

    // Finally, rewrite patched file
    out.seekp(0);
    out.write(_buffer, _bufferSize);
    out.close();

    delete[] _buffer;

    return Success;
}

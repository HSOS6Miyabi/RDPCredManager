#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using Bytes = std::vector<unsigned char>;

std::uint16_t U16(const Bytes& b, std::size_t pos) {
    if (pos > b.size() || b.size() - pos < 2) throw std::runtime_error("PE data truncated");
    return std::uint16_t(b[pos] | (std::uint16_t(b[pos + 1]) << 8));
}
std::uint32_t U32(const Bytes& b, std::size_t pos) {
    if (pos > b.size() || b.size() - pos < 4) throw std::runtime_error("PE data truncated");
    return std::uint32_t(b[pos]) | (std::uint32_t(b[pos + 1]) << 8) |
           (std::uint32_t(b[pos + 2]) << 16) | (std::uint32_t(b[pos + 3]) << 24);
}
std::size_t InRange(const Bytes& b, std::size_t pos, std::size_t count) {
    if (pos > b.size() || count > b.size() - pos) throw std::runtime_error("PE data truncated");
    return pos;
}
void Require(bool condition, const char* explanation) {
    if (!condition) throw std::runtime_error(explanation);
}
struct Section {std::uint32_t address, virtualSize, rawOffset, rawSize;};
std::size_t OffsetForRva(const Bytes& b, const std::vector<Section>& sections, std::uint32_t rva) {
    for (const Section& s : sections) {
        std::uint64_t n = std::max(s.virtualSize, s.rawSize);
        if (rva >= s.address && std::uint64_t(rva - s.address) < n) {
            std::uint32_t delta = rva - s.address;
            Require(delta < s.rawSize, "Resource RVA refers to uninitialized data");
            return InRange(b, std::size_t(s.rawOffset) + delta, 1);
        }
    }
    throw std::runtime_error("Resource RVA not mapped to a PE section");
}
std::string EmbeddedManifest(const Bytes& b, const std::string& expectedArchitecture) {
    Require(U16(b, 0) == 0x5a4d, "DOS header missing");
    std::size_t pe = U32(b, 0x3c);
    Require(U32(b, pe) == 0x00004550, "PE header missing");
    const std::uint16_t machine=U16(b, pe + 4);
    const std::uint16_t expectedMachine=expectedArchitecture=="x86" ? 0x014c :
        expectedArchitecture=="x64" ? 0x8664 :
        expectedArchitecture=="arm64" ? 0xaa64 : 0;
    Require(expectedMachine!=0, "Unknown architecture: expected x86, x64 or arm64");
    Require(machine==expectedMachine, "PE machine type does not match the expected architecture");
    std::uint16_t count = U16(b, pe + 6);
    std::uint16_t optionalSize = U16(b, pe + 20);
    std::size_t opt = pe + 24;
    const bool is32=expectedArchitecture=="x86";
    Require(U16(b, opt)==(is32 ? 0x10b : 0x20b),
            "PE optional header format does not match target architecture");
    Require(U16(b, opt + 68) == 2, "Windows graphical subsystem required");
    const std::size_t dirCountOffset=is32?92:108;
    const std::size_t dirStartOffset=is32?96:112;
    Require(optionalSize >= dirStartOffset+24, "PE optional header is incomplete");
    Require(U32(b, opt + dirCountOffset) >= 3, "PE lacks resource directory entry");
    std::uint32_t resourcesRva = U32(b, opt + dirStartOffset + 2 * 8);
    Require(resourcesRva != 0, "PE resource directory missing");
    std::vector<Section> sections;
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t at = InRange(b, opt + optionalSize + i * 40, 40);
        sections.push_back({U32(b, at + 12), U32(b, at + 8), U32(b, at + 20), U32(b, at + 16)});
    }
    std::size_t root = OffsetForRva(b, sections, resourcesRva);
    auto childDirectory = [&](std::size_t dir, int wantedId) -> std::size_t {
        std::uint16_t named = U16(b, dir + 12), ids = U16(b, dir + 14);
        Require(std::uint32_t(named) + ids < 65536, "Invalid resource directory");
        for (std::size_t i = 0; i < std::size_t(named) + ids; ++i) {
            std::size_t at = InRange(b, dir + 16 + i * 8, 8);
            auto id = U32(b, at);
            if (!(id & 0x80000000u) && int(id) == wantedId) {
                auto target = U32(b, at + 4);
                Require((target & 0x80000000u) != 0, "Resource directory level invalid");
                return InRange(b, root + (target & 0x7fffffffu), 16);
            }
        }
        throw std::runtime_error("Application manifest resource not found");
    };
    std::size_t names = childDirectory(root, 24); // RT_MANIFEST
    std::size_t languages = childDirectory(names, 1); // Process manifest
    Require(U16(b, languages + 12) + U16(b, languages + 14) > 0,
            "Manifest has no resource language");
    std::size_t leaf = InRange(b, languages + 16, 8);
    std::uint32_t entry = U32(b, leaf + 4);
    Require((entry & 0x80000000u) == 0, "Manifest leaf is not resource data");
    std::size_t record = InRange(b, root + entry, 16);
    std::uint32_t dataRva = U32(b, record), length = U32(b, record + 4);
    Require(length > 0 && length < 1024 * 1024, "Manifest has invalid length");
    std::size_t start = OffsetForRva(b, sections, dataRva);
    InRange(b, start, length);
    return std::string(b.begin() + start, b.begin() + start + length);
}
void Verify(const std::string& text) {
    auto contains = [&](const char* needle) {return text.find(needle) != std::string::npos;};
    Require(contains("<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\""),
            "Assembly namespace missing");
    Require(contains("<trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\""),
            "UAC manifest namespace invalid");
    Require(contains("<requestedExecutionLevel level=\"asInvoker\" uiAccess=\"false\""),
            "UAC execution level attributes invalid");
    Require(!contains("ms_asmv1:level") && !contains("ms_asmv1:uiAccess"),
            "UAC attributes have namespace prefixes");
    Require(contains("Microsoft.Windows.Common-Controls") && contains("version=\"6.0.0.0\""),
            "Common Controls v6 dependency missing");
    Require(contains("PerMonitorV2"), "Per-monitor V2 DPI awareness missing");
    Require(contains("processorArchitecture=\"*\""), "Architecture-neutral manifest identity missing");
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 3, "Usage: manifest_verify executable.exe x86|x64|arm64");
        std::ifstream input(argv[1], std::ios::binary);
        Require(input.good(), "Cannot open Windows executable");
        Bytes data{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        Verify(EmbeddedManifest(data, argv[2]));
        std::cout << "PE embedded manifest: verified\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Manifest validation failed: " << e.what() << '\n';
        return 1;
    }
}

#ifndef EGALITO_GENERATE_SEGMENT_H
#define EGALITO_GENERATE_SEGMENT_H

#include <string>
#include <vector>
#include <elf.h>
#include "types.h"

class Section;

class Segment {
private:
    address_t address;
    size_t fileOffset;
    std::vector<Section *> sections;
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Xword p_align;
public:
    // Sometimes the first parameter will be ignored, because the Segment
    // won't be mapped into memory
    Segment() : address(0), fileOffset(0), p_type(PT_NULL),
        p_flags(PT_NULL), p_align(PT_NULL) {}
    ~Segment();
public:
    address_t getAddress() const { return address; }
    size_t getFileOff() const { return fileOffset; }
    size_t getSize();
    std::vector<Section *> getSections() const { return sections; }
    Section *getFirstSection() const { return sections[0]; }
public:
    void setAddress(address_t addr);
    void setFileOff(size_t offset);
    void setPhdrInfo(Elf64_Word ptype, Elf64_Word pflags, Elf64_Xword palign);
public:
    friend std::ostream& operator<<(std::ostream &stream, Segment &rhs);
    void add(Section *sec);
    Elf64_Phdr *makePhdr();
};

#endif

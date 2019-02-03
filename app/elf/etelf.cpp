#include <iostream>
#include <functional>
#include <string>
#include <cstring>  // for std::strcmp
#include "etelf.h"
#include "conductor/conductor.h"
#include "conductor/setup.h"
#include "pass/collapseplt.h"
#include "pass/promotejumps.h"
#include "pass/ldsorefs.h"
#include "pass/ifuncplts.h"
#include "pass/fixenviron.h"
#include "pass/externalsymbollinks.h"
#include "log/registry.h"
#include "log/temp.h"

static void parse(const std::string& filename, const std::string& output, bool oneToOne, bool quiet) {
    ConductorSetup setup;
    std::cout << "Transforming file [" << filename << "]\n";

    if(quiet) {
        GroupRegistry::getInstance()->muteAllSettings();
    }

    try {
        if(ElfMap::isElf(filename.c_str())) {
            std::cout << "Parsing ELF file and all shared library dependencies...\n";
            setup.parseElfFiles(filename.c_str(), /*recursive=*/ !oneToOne, false);
        }
        else {
            std::cout << "Parsing archive...\n";
            setup.parseEgalitoArchive(filename.c_str());
        }

        auto program = setup.getConductor()->getProgram();

        std::cout << "Preparing for codegen...\n";
        if(oneToOne) {
            CollapsePLTPass collapsePLT(setup.getConductor());
            program->accept(&collapsePLT);

            PromoteJumpsPass promoteJumps;
            program->accept(&promoteJumps);

            //ExternalSymbolLinksPass externalSymbolLinks;
            //program->accept(&externalSymbolLinks);
        }
        else {
            FixEnvironPass fixEnviron;
            program->accept(&fixEnviron);

            CollapsePLTPass collapsePLT(setup.getConductor());
            program->accept(&collapsePLT);

            PromoteJumpsPass promoteJumps;
            program->accept(&promoteJumps);
        }

        if(oneToOne) {
            // generate mirror executable.
            std::cout << "Generating mirror executable [" << output << "]...\n";
            LdsoRefsPass ldsoRefs;
            program->accept(&ldsoRefs);

            ExternalSymbolLinksPass externalSymbolLinks;
            program->accept(&externalSymbolLinks);

            IFuncPLTs ifuncPLTs;
            program->accept(&ifuncPLTs);

            setup.generateMirrorELF(output.c_str());
        }
        else {
            // generate static executable.
            std::cout << "Generating union executable [" << output << "]...\n";
            LdsoRefsPass ldsoRefs;
            program->accept(&ldsoRefs);
            IFuncPLTs ifuncPLTs;
            program->accept(&ifuncPLTs);

            setup.generateStaticExecutable(output.c_str());
        }
    }
    catch(const char *message) {
        std::cout << "Exception: " << message << std::endl;
    }
}

static void printUsage(const char *program) {
    std::cout << "Usage: " << program << " [options] input-file output-file\n"
        "    Transforms an executable to a new ELF file.\n"
        "\n"
        "Options:\n"
        "    -m     Perform mirror elf generation (1-1 output)\n"
        "    -u     Perform union elf generation (merged output)\n"
        "    -v     Verbose mode, print logging messages\n"
        "    -q     Quiet mode (default), suppress logging messages\n"
        "Note: the EGALITO_DEBUG variable is also honoured.\n";
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        printUsage(argv[0] ? argv[0] : "etelf");
        return 0;
    }

    if(!SettingsParser().parseEnvVar("EGALITO_DEBUG")) {
        printUsage(argv[0]);
        return 1;
    }

    bool oneToOne = false;
    bool quiet = true;

    struct {
        const char *str;
        std::function<void ()> action;
    } actions[] = {
        // which elf gen should we perform?
        {"-m", [&oneToOne] () { oneToOne = true; }},
        {"-u", [&oneToOne] () { oneToOne = false; }},

        // should we show debugging log messages?
        {"-v", [&quiet] () { quiet = false; }},
        {"-q", [&quiet] () { quiet = true; }},
    };

    for(int a = 1; a < argc; a ++) {
        const char *arg = argv[a];
        if(arg[0] == '-') {
            bool found = false;

            for(auto action : actions) {
                if(std::strcmp(arg, action.str) == 0) {
                    action.action();
                    found = true;
                    break;
                }
            }
            if(!found) {
                std::cout << "Warning: unrecognized option \"" << arg << "\"\n";
            }
        }
        else if(argv[a] && argv[a + 1]) {
            parse(argv[a], argv[a + 1], oneToOne, quiet);
            break;
        }
        else {
            std::cout << "Error: no output filename given!\n";
            break;
        }
    }
    return 0;
}
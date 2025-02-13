#include "file.hpp"
#include <cstdio>


void help(char* progname) {
	// clang-format off
	printf(
	"VPNGate .dat ID utility\n"
			"Usage: %s [path to VPNGate .dat file]\n",
			progname
	);
	// clang-format on
}

int main(int argc, char** argv) {
	if(argc != 2) {
		help(argv[0]);
		return 0;
	}

	if(std::string_view(argv[1]) == "--help") {
		help(argv[0]);
		return 0;
	}

	auto dat = File::Open(argv[1], O_RDONLY);

    if(dat.ReadLine() != "[VPNGate Data File]")  {
        std::printf("This file is NOT a VPNGate dat file\n");
        return 1;
    }

    auto datId = dat.ReadLine();
    std::printf("%s\n", datId.c_str());
    return 0;
}
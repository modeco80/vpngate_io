#include <vpngate_io/simple.hpp>
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

	vpngate_io::Simple simple(argv[1]);

	switch(simple.Init()) {
		case vpngate_io::SimpleErrc::Ok: break;
		case vpngate_io::SimpleErrc::InvalidDat: {
			printf("\"%s\" does not appear to be a VPNGate.dat file.\n", argv[1]);
			return 1;
		}; break;
	}

    std::printf("%s\n", simple.GetIdentifier().c_str());
    return 0;
}
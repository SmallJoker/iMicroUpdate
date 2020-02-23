#include "args_parser.h"

#include <fstream>
#include <iostream>

#define LOG(msg) \
	std::cout << msg << std::endl;

#define CHECK(cond, err_msg) \
	if (!(cond)) { \
		std::cout << err_msg << std::endl; \
		return 1; \
	}

// Little endian
struct MicrocodeInfo {
	uint64_t pos;
	uint16_t revision; // 0x04
	uint32_t cpuid;   // 0x0C
	uint8_t platform; // 0x18
	uint16_t size;    // 0x20 + 0x21

	static void printHeader()
	{
		printf(" Offset |  Size  | CPUID | Pf | Rev\n");
		printf("--------+--------+-------+----+----\n");
	}

	void dump()
	{
		printf("0x%05lX - 0x%04X | %5X | %02X | %3X\n",
			pos, size, cpuid, platform, revision);
	}
};

uint16_t read2Bytes(std::istream *file)
{
	static uint8_t buffer[2];
	file->read((char *)buffer, 2);
	if (!file->good())
		return 0;

	return buffer[0] | buffer[1] << 8;
}

bool readMicrocode(std::istream *file, MicrocodeInfo *info = nullptr)
{
	int64_t pos = file->tellg();
	if ((pos & 0x0FL) != 0)
		return false; // Aligned à 16 bytes

	char buffer[0x18];
	file->read(buffer, 0x18);
	file->seekg(pos);

	if (!(buffer[0x00] == 0x01
			&& buffer[0x01] == 0x00
			&& buffer[0x02] == 0x00
			&& buffer[0x03] == 0x00
			&& buffer[0x14] == 0x01
			&& buffer[0x15] == 0x00
			&& buffer[0x16] == 0x00
			&& buffer[0x17] == 0x00))
		return false;

	if (!info)
		return true;

	info->pos = pos;
	file->seekg(pos + 0x04);
	info->revision = read2Bytes(file);
	file->seekg(pos + 0x0C);
	info->cpuid = read2Bytes(file) | read2Bytes(file) << 16;
	file->seekg(pos + 0x18);
	info->platform = file->get();
	file->seekg(pos + 0x20);
	info->size = read2Bytes(file);

	if (info->size == 0)
		info->size = 0x800; // Default?

	return true;
}

int scanFile(const std::string &filename, int64_t pos)
{
	const int64_t STEPSIZE_NORMAL = 0x10;

	std::ifstream romfile(filename, std::ios_base::binary | std::ios_base::ate);
	CHECK(romfile.is_open(), "ROM: File not found");

	int64_t bin_size = romfile.tellg();
	char peek;
	MicrocodeInfo info;
	int64_t last_pos = 0;

	info.printHeader();
	while (pos < bin_size) {
		romfile.seekg(pos);
		romfile.get(peek);
		romfile.unget();

		if (peek == 0x01 && readMicrocode(&romfile, &info)) {
			if (pos != last_pos) {
				printf("Unknown region: 0x%lX bytes from 0x%lX to 0x%lX\n",
					 pos - last_pos, last_pos, pos - 1);
			}
			info.dump();
			pos += (int64_t)info.size;
			last_pos = pos;
		} else {
			pos += STEPSIZE_NORMAL;
		}
	}
	return 0;
}

int patchFile(CONSTSTR rompath, CONSTSTR binpath, int64_t rompos, int64_t binpos)
{
	std::cout << std::hex;
	LOG("\tROM file:   " << rompath << " @ 0x" << rompos);
	LOG("\tMicrocode:  " << binpath << " @ 0x" << binpos);

	// Open and check files
	std::fstream romfile(rompath,
		std::ios_base::in | std::ios_base::out | std::ios_base::binary);
	CHECK(romfile.is_open(), "ROM: File not found");
	std::ifstream binfile(binpath, std::ios_base::binary | std::ios_base::ate);
	CHECK(binfile.is_open(), "BIN: File not found");

	// Move to write position
	auto bin_size = binfile.tellg();
	romfile.seekg(rompos);
	binfile.seekg(binpos);
	MicrocodeInfo old_uc, new_uc;
	CHECK(readMicrocode(&romfile, &old_uc), "ROM: No microcode at pos 0x" << romfile.tellg());
	CHECK(readMicrocode(&binfile, &new_uc), "BIN: No microcode at pos 0x" << binfile.tellg());
	CHECK(bin_size == new_uc.size, "BIN: Invalid file length");
	LOG("Old vs new microcode:");
	old_uc.printHeader();
	old_uc.dump();
	new_uc.dump();
	LOG("Please ENTER to confirm.");
	getchar();

	char *buffer = new char[bin_size];
	binfile.read(buffer, bin_size);
	romfile.write(buffer, bin_size);
	delete[] buffer;

	binfile.close();
	romfile.close();

	LOG("DONE. Patched.");
	return 0;
}

int main(int argc, char **argv)
{
	LOG("iMicroUpdate - Microcode updater for Intel BIOSes");
	LOG("  Patch: -patch ROMFILE -bin MICROCODE -posp 0 [-posb 0]");
	CLIArgStr ca_romfile("patch", "ROMFILE.ROM");
	CLIArgStr ca_binfile("bin", "MICROCODE.BIN");
	CLIArgS64 ca_rompos("posp", 0);
	CLIArgS64 ca_binpos("posb", 0);

	LOG("  Scan:  -scan FILE [-posr 0]");
	CLIArgStr ca_scanfile("scan", "");
	CLIArg::parseArgs(argc, argv);
	LOG("");

	if (!ca_scanfile.get().empty())
		return scanFile(ca_scanfile.get(), ca_rompos.get());

	if (!ca_romfile.get().empty())
		return patchFile(
			ca_romfile.get(), ca_binfile.get(),
			ca_rompos.get(), ca_binpos.get());

	LOG("Nothing to do. Check inputs.");
	return 1;
}

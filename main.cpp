#include "args_parser.h"

#include <fstream>
#include <iostream>
#include <cstring> // memset

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
		printf(" CPUID | Pf | Rev | Offset  | Size\n");
		printf("-------+----+-----+---------+--------\n");
	}

	void dump()
	{
		printf(" %5X | %02X | %3X | 0x%05lX | 0x%04X\n",
			cpuid, platform, revision, pos, size);
	}

	void toFileName(std::string &file)
	{
		file.resize(256);
		int len = snprintf(&file[0], file.size(), "cpu%5X_plat%02X_rev%04X.bin",
			cpuid, platform, revision);
		file.resize(len);
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

	file->seekg(pos);
	return true;
}

int extractFile(CONSTSTR rompath, int64_t pos, std::string binpath)
{
	std::ifstream romfile(rompath, std::ios_base::binary);
	CHECK(romfile.is_open(), "ROM: File not found");

	romfile.seekg(pos);
	MicrocodeInfo info;
	CHECK(readMicrocode(&romfile, &info), "ROM: No microcode at pos 0x" << romfile.tellg());

	info.printHeader();
	info.dump();

	if (binpath.empty()) // Generate one
		info.toFileName(binpath);

	std::ofstream binfile(binpath, std::ios_base::binary | std::ios_base::trunc);
	CHECK(binfile, "BIN: Cannot create & open file.");

	char *data = new char[info.size];
	romfile.read(data, info.size);
	binfile.write(data, info.size);
	delete[] data;
	binfile.flush();

	LOG("Saved to '" << binpath << "'");
	return 0;
}

int scanFile(CONSTSTR filename, int64_t pos)
{
	std::ifstream romfile(filename, std::ios_base::binary | std::ios_base::ate);
	CHECK(romfile.is_open(), "ROM: File not found");

	int64_t bin_size = romfile.tellg();
	char peek;
	MicrocodeInfo info;
	int64_t pos_end = 0;

	info.printHeader();
	while (pos < bin_size) {
		romfile.seekg(pos);
		romfile.get(peek);
		romfile.unget();

		if (peek == 0x01 && readMicrocode(&romfile, &info)) {
			if (pos != pos_end) {
				printf("\tUnknown region: 0x%lX to 0x%lX (0x%lX bytes)\n",
					pos_end, pos - 1, pos - pos_end);
			}
			info.dump();
			pos += (int64_t)info.size;
			pos_end = pos;
		} else {
			// Speed optimization. Gaps are usually multiples of 256 bytes
			pos += pos_end > 0 ? 0x100 : 0x04;
		}
	}
	if (pos_end != bin_size)
		printf("\tUnknown region: 0x%lX to 0x%lX (0x%lX bytes) [EOF]\n",
					pos_end, pos - 1, pos - pos_end);
	else
		printf("\tNo unknown region [EOF]\n");
	return 0;
}

int patchFile(CONSTSTR rompath, CONSTSTR binpath,
	int64_t rompos, int64_t binpos, bool force)
{
	std::cout << std::hex;
	LOG("\tROM file:   " << rompath);
	LOG("\tMicrocode:  " << binpath);

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

	// Read and check microcodes
	MicrocodeInfo old_uc, new_uc;
	bool microcode_ok = readMicrocode(&romfile, &old_uc);
	CHECK(microcode_ok || force, "ROM: No microcode at pos 0x" << romfile.tellg());
	CHECK(readMicrocode(&binfile, &new_uc), "BIN: No microcode at pos 0x" << binfile.tellg());

	// Verify data lengths
	CHECK(bin_size - binpos >= new_uc.size, "BIN: Invalid file length");
	CHECK(romfile.tellg() == rompos, "Sanity check failed");
	bin_size = new_uc.size;

	LOG("\nOld vs new microcode:");
	old_uc.printHeader();
	if (microcode_ok)
		old_uc.dump();
	else
		LOG("  -- NONE OR INVALID MICROCODE --"); // Forced

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

int eraseFile(CONSTSTR rompath, int64_t pos, int64_t fill)
{
	CHECK(fill >= 0 && fill <= 0xFF, "Fill byte is out of range");

	std::fstream romfile(rompath,
		std::ios_base::in | std::ios_base::out | std::ios_base::binary);
	CHECK(romfile.is_open(), "ROM: File not found");

	// Move to write position
	romfile.seekg(pos);

	// Read and check microcodes
	MicrocodeInfo old_uc;
	bool microcode_ok = readMicrocode(&romfile, &old_uc);
	CHECK(microcode_ok, "ROM: No microcode at pos 0x" << romfile.tellg());

	LOG("\nMicrocode to remove:");
	old_uc.printHeader();
	old_uc.dump();

	LOG("Please ENTER to confirm.");
	getchar();

	char *buffer = new char[old_uc.size];
	memset(buffer, (uint8_t)fill, old_uc.size);
	romfile.write(buffer, old_uc.size);
	delete[] buffer;

	romfile.close();

	LOG("DONE. Removed microcode.");
	return 0;
}

int main(int argc, char **argv)
{
	LOG("iMicroUpdate - Updater tool for Intel CPU microcode in BIOSes");
	LOG("");

	// Scan
	CLIArgStr ca_scanfile("scan", "");
	CLIArg::parseArgs(argc, argv);

	// Extract
	CLIArgStr ca_extractfile("extract", "");
	CLIArg::parseArgs(argc, argv);

	// Patch
	CLIArgStr ca_patchfile("patch", "");
	CLIArgStr ca_binfile("bin", "");
	CLIArgS64 ca_rompos("posr", 0);
	CLIArgS64 ca_binpos("posb", 0);
	CLIArgFlag ca_force("force");

	// Erase
	CLIArgStr ca_erasefile("erase", "");
	CLIArgS64 ca_fill("fill", 0);
	CLIArg::parseArgs(argc, argv);

	if (!ca_extractfile.get().empty())
		return extractFile(ca_extractfile.get(), ca_rompos.get(),
			ca_binfile.get());

	if (!ca_scanfile.get().empty())
		return scanFile(ca_scanfile.get(), ca_rompos.get());

	if (!ca_patchfile.get().empty())
		return patchFile(
			ca_patchfile.get(), ca_binfile.get(),
			ca_rompos.get(), ca_binpos.get(), ca_force.get());

	if (!ca_erasefile.get().empty())
		return eraseFile(ca_erasefile.get(), ca_rompos.get(),
			ca_fill.get());

	LOG("  Scan:    -scan FILE [-posr 0]");
	LOG("  Extract: -extract ROMFILE [-bin DESTINATION] [-posr 0]");
	LOG("  Patch:   -patch ROMFILE -bin MICROCODE -posr 0 [-posb 0] [-force]");
	LOG("  Erase:   -erase ROMFILE [-posr 0] [-fill 0]");
	LOG("");
	return 1;
}

# iMicroUpdate

A tool to brick your BIOS less easily.
This is an experimental application that is capable of replacing Intel microcodes in BIOS ROM files.
It is intended to be a replacement for manual copy & paste in hex editors.

License: MIT

## Command examples

### Scan file

This command lists all microcodes and gaps that could be found within the specified file.

	-scan MYROM.ROM


### Extract microcode

Scan the file to know the right offset in the ROM file. Afterwards:

	-extract MYROM.ROM -posr 0x23830 -bin old_9110676.BIN


### Patch ROM file

This command will modify the ROM file at the given offset by the provided microcode:

	-patch MYROM.ROM -posr 0x23830 -bin 9110676.BIN

It is also possible to use another ROM as microcode source:

	-patch MYROM.ROM -posr 0x23830 -bin IntelAll.BIN -posb 0x32000

In case the microcode sizes mismatch or no microcode is found in the ROM file (gap?),
then consider using `-force`.

WARNING: Ensure there is enough space else it will overwrite adjacent microcodes!

	-patch MYROM.ROM -posr 0x23830 -bin new10676.bin -posb 0x32000 -force

**Example:**

	$ ./iMicroUpdate -patch MYROM.ROM -bin 9110676.BIN -posr 0x23830
	
		ROM file:   MYROM.ROM
		Microcode:  9110676.BIN
	
	Old vs new microcode:
	 CPUID | Pf | Rev | Offset  | Size
	-------+----+-----+---------+--------
	 10676 | 01 | 60C | 0x23830 | 0x1000
	 10676 | 44 | 60F | 0x00000 | 0x1000
	Please ENTER to confirm.
	
	DONE. Patched.


### Erase microcode

Replace a microcode with a gap. This command is helpful to free space to
insert larger microcodes. Consider `-erase` prior `-patch ... -force`.

	-erase MYROM.ROM -posr 0x23830 -fill 0x00

The gap is usually filled with `0x00` (AMI BIOS).


## Features

- Scan, extract and replace microcodes
- Cross-platform application
- Script-friendly return code

See also: [CPUMicrocode](https://github.com/platomav/CPUMicrocodes/) (microcode collection)


## Tested

- AMI BIOS: Filling microcode gaps (`0x400` bytes of `0x00`)
- AMI BIOS: Altered count of microcodes
	- Example: 3 `0xC00+0x800+0xC00` to 1 `0x2000`
	- Needs use of `-force` if regions get split
- Phoenix BIOS (`_C00.PEI): Altered count of microcodes
	- Use PhoenixTool (primary for SLIC) to extract & rebuild the `*.wph` file

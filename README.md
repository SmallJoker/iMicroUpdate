# iMicroUpdate

A tool to brick your BIOS less easily.
This is an experimental application that is capable of replacing Intel microcodes in BIOS ROM files.
It is intended to be a replacement for manual copy & paste and `dd`.

License: MIT

	$ ./iMicroUpdate -patch MYROM.ROM -bin 9110676.BIN -posp 0x23830
	iMicroUpdate - Microcode updater for Intel BIOSes
	  Patch: -patch ROMFILE -bin MICROCODE -posp 0 [-posb 0]
	  Scan:  -scan FILE [-posr 0]
	
		ROM file:   MYROM.ROM
		Microcode:  9110676.BIN
	
	Old vs new microcode:
	 CPUID | Pf | Rev | Offset  | Size
	-------+----+-----+---------+--------
	 10676 | 01 | 60C | 0x23830 | 0x1000
	 10676 | 44 | 60F | 0x00000 | 0x1000
	Please ENTER to confirm.
	
	DONE. Patched.


### Features

- Scan ROM files (`-scan FILE`)
- Replace microcodes (`-patch`)
- Cross-platform application

Microcodes can be found here: [CPUMicrocode](https://github.com/platomav/CPUMicrocodes/)


### Tested

- AMI BIOS: Filling microcode gaps (`0x400` bytes of `0x00`)
- AMI BIOS: Altered count of microcodes
	- Example: 3 `0xC00+0x800+0xC00` to 1 `0x2000`
	- Needs use of `-force` if regions get split

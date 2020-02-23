# iMicroUpdate

A tool to brick your BIOS easily.
This is an experimental application that is capable of replacing Intel microcodes in BIOS ROM files.
It is intended to be a replacement for manual copy & paste and `dd`.

License: MIT

	$ ./iMicroUpdate -patch MYROM.ROM -bin 9110676.PAT -posp 0x52540
	MicroUpdate - Microcode updater for Intel BIOSes
	  Patch: -patch ROMFILE -bin MICROCODE -posp 0 [-posb 0]
	  Scan:  -scan FILE [-posr 0]
	
		ROM file:   MYROM.ROM @ 0x52540
		Microcode:  9110676.PAT @ 0x0
	Old vs new microcode:
	 Offset |  Size  | CPUID | Pf | Rev
	--------+--------+-------+----+----
	0x52540 - 0x1000 | 10676 | 91 | 612
	0x00000 - 0x1000 | 10676 | 91 | 612
	Please ENTER to confirm.
	
	DONE. Patched.


### Features

- Scan ROM files (`-scan FILE`)
- Replace microcodes (`-patch`)
- Cross-platform application

Microcodes can be found here: [CPUMicrocode](https://github.com/platomav/CPUMicrocodes/)


### Untested

- Everything but AMI BIOS on ASUS mainboards
- Altered count of microcodes
	- Example: 3 `0xC00+0x800+0xC00` to 1 `0x2000`

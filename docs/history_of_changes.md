TAPClean - History of changes
=============================

Entries are of the following types, since release 0.21:

- Code quality: changes that make the code more readable, portable, or maintainable
- Fix: bug fixes
- Improvement: where end-user experience has been improved (or frustration minimized)
- New: new features, such as support for additional loaders

v0.38 (WIP)
-----------

- Improvement: The first non-standard 3-page "Enigma" file in "Rik The Roadie", supplied by Paul Jones, is now fully acknowledged - only 2 pages of such file are loaded in RAM (Luigi)
- New: Added support for "Turbotape 526" (a.k.a. "Load'n'Run"), a slow clone of "Turbotape 250", used in a number of titles supplied by Walter A. (Luigi)
- New: Added support for "Microload Blue Ribbon Variant", a more flexible version of "Microload", used in a number of titles supplied by Paul Jones (Luigi)
- Fix: Added the missing entry for "Glass Tape" to the `knam` array (Luigi)
- Improvement: The "Microload" scanner was rewritten (Luigi)

v0.37
-----

- Improvement: The filename occurring within a "Turbotape 250" header file is also embedded in the PRG filename of its data counterpart (Luigi)
- Improvement: Added signature of the "Ocean New F2" instances used by "Hero Quest", and "World Championship Boxing Manager" (Luigi)
- Improvement: The "Bleepload" scanner was rewritten (Luigi)
- Code quality: Consistently refer to "block number" as such, instead of also using "block ID" or "file ID" (Luigi)
- Improvement: The "Bleepload Special" scanner was rewritten (Luigi)
- Improvement: Added a number of signatures for "Bleepload" titles in order to speedup scanning (Luigi)
- Improvement: The "Bleepload" scanner now supports multiple titles on the same tape image (Luigi)
- Improvement: Added signature of the "Power Load" instance used by "Mini Office", supplied by Paul Jones (Luigi)
- Improvement: The "Power Load" scanner now supports multiple titles on the same tape image (Luigi)
- Improvement: Added signature of the "Power Load" instance used by "World Cup II", supplied by Paul Jones (Luigi)
- Improvement: Added signature of the "Power Load" variant used by "Bombo", "Ian Botham's Test Match", and "Euro Games 64", supplied by Paul Jones (Luigi)
- Improvement: Added initial multi-turbo-file support for "Power Load" (Luigi)
- Improvement: Added signature of the "Ocean New F2" instance used by "Jahangir Khan World Championship Squash", supplied by Paul  Jones (Luigi)
- New: Added support for "Glass Tape", used in a number of titles supplied by Paul Jones (Luigi)
- Fix: Corrected offset in the error message within the "Turrican" scanner (Luigi)
- Improvement: Improved filename length detection in the "American Action" scanner for dealing with "Point X", submitted by Kim L. (Luigi)
- Improvement: Added signature of the "Power Load" instance used by "Lighforce", supplied by Paul Jones (Luigi)
- New: Added support for "Mega-Save T4", used in "Action Pack 2", supplied by Paul Jones (Luigi)
- Improvement: The "Fast Evil" scanner was renamed to "Jiffy Load T1" based on reports and sample tapes supplied by Paul Jones (Luigi)
- New: Added support for "Jiffy Load T2", used in a few titles by "Solar Software", supplied by Paul Jones (Luigi)

v0.36
-----

- Improvement: The "CHR" scanner was rewritten and renamed to "Mega-Save" (Luigi)
- Code quality: Implemented a number of rationalisations across the board (Luigi)
- Improvement: Consolidated "Ocean New F1" T1/T2 scanners into a single one, entirely rewritten (Luigi)
- Improvement: Consolidated "Snakeload 5.0" T1/T2 scanners into a single one, entirely rewritten (Luigi)
- Code quality: Simplified the sync sequence matching logic in a number of scanners (Luigi)
- Code quality: Modularised the Tx scan in "Mega-Save", "Freeload Slowload", "Alternative SW (DK)", and "MSX Tape" (Luigi)
- Improvement: The "Freeload" scanner was rewritten (Luigi)
- Improvement: Added support for "Barbarian II" in "Palace Tape F1" (Luigi)

v0.35
-----

- Improvement: The "Visiload" scanner now scans CBM files in order to extract loading parameters and supports multiple titles on the same tape image even when these use different initial loading parameters, including different threshold values (Luigi)
- New: Added support for "Visiload T6", used in "Spindizzy 64", supplied by Paul Jones (Luigi)
- Improvement: "Wildload" scanner rewritten (Luigi)
- New: Added support for "Visiload T7", used in "Puffy's Saga", supplied by Paul Jones (Luigi)
- Improvement: The "Pavloda" scanner does not alter TAP data in memory any longer at the search stage: Melon Mania (Microload) does not fail checkbyte test of the file loaded at $3000 (Luigi)

v0.34
-----

- Improvement: Rationalised usage and options' messages (Luigi)
- New: Added the "-reckless" option to clean tape images with read errors (Luigi)
- Code quality: Made a number of rationalisations (Luigi)
- Improvement: "Burner" scanner rewritten (Luigi)
- Code quality: Rationalised a number of scanners along with file detail messages (Luigi)
- Code quality: Simplified reading and checking of the post-data pattern in "FastEvil" scanner (Luigi)
- Improvement: "Cyberload F3" now shows the execution address when in use (Luigi)
- Improvement: "Hi-Tech" and "Virgin" scanners rewritten (Luigi)
- Improvement: Revised ideal pulsewidths for "Alternative Software  (DK) T3", used in "Arcade Classics" and "Special Agent", supplied  by Paul Jones (Luigi)
- Improvement: Added signature of the "Alternative Software  (DK) T3" instance used by "Special Agent" (Luigi)
- New: Added support for "Alternative Software (DK) T4", used in "Interdictor Pilot", supplied by Paul Jones (Luigi)
- New: Added support for "Visiload T5", used in "Critical Mass",  "Spellseeker", "Confuzion", "Nick Faldo Plays  The Open",  "TimeTrax", "Mr. Mephisto", supplied by Paul Jones (Luigi)
- Improvement: The special modifier logic used in "Narco Police"'s "Visiload" is now only used when the CBM filename starts with "NARCO POLICE" in order not to trigger false positives in e.g. "Critical Mass" (Luigi)
- Improvement: The "Turbotape 250" scanner can now handle three versions of "Micrus Copy" files, popular in cracked Polish tapes, submitted by Pawel (Luigi)
- New: Added support for "Freeload Slowload T2", used in "Super  Champs", supplied by Paul Jones (Luigi)
- Fix: Fixed decoding of the "Turbotape 250" header so that the last byte of the filename is now properly set. Bug reported by Pawel (Luigi)

v0.33
-----

- New: Added initial support for MSX tape Header files (Luigi)
- New: Added initial support for MSX tape Data files (Luigi)
- Code quality: Removed variables that were set but not used (Luigi)
- Fix: Fixed the address at which the Tequila Sunrise PRG decoder starts extraction (Luigi)
- Improvement: Use signature in "CBM Data" for "Trilogic" too (Luigi)
- Improvement: Added sequence numbers to the report (Luigi)
- Improvement: Added signature of the "Power Load" instance used by "Mr Wiz", "Smuggler", "U.K. Geography", "World Geography", and "Percy Penguin" (Luigi)
- Improvement: The "Power Load" scanner now fully supports "Mr Wiz", "Smuggler", and "U.K. Geography" (Luigi)
- Improvement: Sequence numbers are now 1-based in order to match file numbering (Luigi)
- New: Added support for "Digital Design Tape", which is a variant of the "Ash+Dave" loader (Luigi)
- New: Added support for "Alternative Software (DK) T3", used in "Arcade Classics", supplied by Zoë (Luigi)
- Improvement: "Turrican" scanner rewritten in order to optionally decode header files and correctly read checkbytes (Luigi)
- Fix: Fixed memory boundary checks when reading in trailer tones in "Bleepload", "Bleepload Special", "Novaload F1", "Novaload F2", "Ocean", "Rasterload", "Snakeload 5.0 T1", "Snakeload 5.0 T2", and "Snakeload 5.1" (Luigi)

v0.32
-----

- Fix: BASIC program start address is $1001 for C16/+4 too (Luigi)
- Improvement: Extract load/end address of the first "Enigma" turbo file (Luigi)
- Improvement: Extract load/end address for all "Enigma" turbo files in non multi-load releases (Luigi)
- Improvement: "Rack-It" scanner rewritten in order to decode and set loader variables that differ between tapes (Luigi)
- Improvement: Overwrite known oversized pulses in "Visiload T2" modifier headers too (Luigi)
- New: Added support for "Rack-It Variant" (Luigi)
- New: Added support for single-file "Creative Sparks" tapes (Luigi)
- Improvement: Acknowledge the last section of partially loaded files as trailer in "ACE 2" (Luigi)
- Improvement: Corrected `supertape_readbyte()` to cater for the fact 8 bits might be made of less than 8 pulses (Luigi)
- Improvement: The cleaning stage can now fix Pavloda check bytes in all scenarios where a single pulse is missing (Luigi)

v0.31
-----

- Improvement: Disabled "Alternative Software" and "Alternative Software DK" scanning in absence of their signature in "CBM Data" (Luigi)
- Fix: Corrected and optimized V0 code in `unify_pauses()` (Luigi)
- Fix: Corrected size of the expected data block in "Audiogenic" scanner (Luigi)
- Improvement: Normalize block names in order to be used for PRG filename generation safely (Luigi)
- Improvement: For "Cyberload F4" use the standard description process to handle block names so that these also propagate into PRG file names (Luigi)
- New: Added support for "Easy-Tape System C" (Luigi)
- Fix: Added plausibility check for "American Action" in order to avoid misrecognition of similar formats (Luigi)
- New: Added support for "Turbo 220" (Luigi)
- Improvement: to stop the irresponsible pollution of TAP archives, files with read errors cannot be cleaned any longer (Luigi)

v0.30
-----

- New: Added initial support for "Ocean New 3" loader (Luigi)
- New: Added initial support for loader variable preservation between program executions (Luigi)
- Fix: Limited the amount of data added to the report buffer in order to avoid access violations (Luigi)

v0.29
-----

- Improvement: Enabled "TES tape" during general scan in order to recognize "Catalypse" (Luigi)
- New: Added support for "Rainbow Islands" loader (Luigi)

v0.28
-----

- Code quality: rewritten aces.c, anirog.c, atlantis.c, and flashload.c (Luigi)
- New: Added support for "American Action" loader (Luigi)
- New: Added support for "Creatures" and "Creatures 2" level files (Luigi)
- Fix: Reported end address inclusive for "CHR", as for other formats (Luigi)
- New: Added support for "Mayhem in Monsterland" level files (Luigi)
- Fix: When there is a read error logged the PRG offset is not off by 2 anymore - applies to new and rewritten scanners (Luigi)

v0.27
-----

- New: boot address of "Power Load" titles is shown inside the report by using the new meta information mechanism (Luigi)
- New: Added support for "Power Load" variant found in "Rocket Roger" (Luigi)
- Improvement: Real end address for "Power Load" blocks is shown inside the report (Luigi)
- New: Added support for "Gremlin F2" variant found in one version of "Footballer Of The Year" (Luigi)
- Fix: file search now works properly under Linux/Unix too during batch scan (Luigi)
- Improvement: PRG filename now contains block index for quick cross- reference (Luigi)
- Fix: Added block header validation for "Freeload" and "Ocean New 2" in order to prevent infinite loops (fabbo)
- Improvement: Added support for loader parameter reset in "Narco Police" (Visiload): it now cleans 100% (Luigi)
- Fix: Added block header validation for "Aces", "Anirog", "Atlantis", "Burner", "CHR", "Firebird", "Flashload", "Hi-Tec", "Hitload", "IK-Loader", "Jetload", "Microload", "Ocean New1 T1", "Ocean New1 T2", "Rasterload", "Snakeload 5.0 T1", "Snakeload 5.0 T2", "Snakeload 5.1", "TDI", "Turbo Tape 250", "Turrican", "U.S. Gold", "Virgin", "Visiload" (Luigi)

v0.26
-----

- Improvement: pilot backtrace in c64tape.c does not step on previously acknowledged files. "World Cup 90"'s CBM repeated blocks are now recognized properly after cleaning (Luigi)
- New: Preview releases are now named v0.x-pre-y; the corresponding CVS tag is v0_x_pre_y (Luigi)
- New: Added support for "Chuckie Egg" loader (Luigi)
- New: Added support for "Alternative Software (DK)" T1 and T2 loaders (Luigi)
- New: Added support for "Power Load" (Luigi)
- New: Added support for "Gremlin F2" (Luigi)
- New: Added support for "Gremlin F1" (Luigi)

v0.25
-----

- New: Use MTAP conversion and Markus' extended TAP format by default when converting from DC2N DMP to TAP (Luigi)
- Fix: proper calculation of end address of "FF Tape" block #3 (Luigi)
- Fix: remove garbage at the beginning of the tape when cleaning (Luigi)
- Fix: Cleaned DMP files have the "tap" extension now (Luigi)
- Fix: Report now shows the cleaned file name after cleaning (Luigi)
- New: Added support for "Tequila Sunrise" loader (Luigi)
- Improvement: Added basic validation of CBM payload size before accessing the `dd[]` array in "Cult", "Mastertronic Burner Variant", and "Biturbo" (Luigi)
- New: Pulse stats per file (Fabbo)
- Fix: end address for pulse stats per file is now correct (Luigi)
- Improvement: pulse stats per file are enabled upon request via the -fstats switch (Luigi)
- Fix: batch report does not show erroneous pulse stats due to per file pulse stats (Luigi)
- Fix: percent signs are properly displayed now, without duplication (Luigi)
- New: Added support for "Alternative Software" loader (Luigi)
- Improvement: Batch scan now looks for DMP files as well (Luigi)
- Code quality: Single Makefile for all architectures (Luigi)

v0.24
-----

- New: Added "skewadapt" patch submitted by Kevin Palberg (Luigi)
- New: Added support for "FF Tape" loader (Luigi)
- New: Added support for "TES tape" loader (Luigi)
- Fix: Tape contents before the first "CBM header" block are not erased anymore (Luigi)
- New: Added Makefile.MAC (bgk)

v0.23
-----

- Fix: excluding "Jetload" doesn't exclude "IK" and viceversa (Luigi)
- Improvement: if occurring in file header "Visiload T1" oversized pulses do not cause scanner to stop looking for Visiload blocks (Luigi)
- Fix: DC2N DMP to TAP conversion now initializes the TAP header (Luigi)
- New: Added support for "Go for the gold" loader, contributed by Kevin  Palberg (Luigi)
- Improvement: "Cyberload F4" scanner now recognizes Myth's F4 files (Kevin)
- Code quality: changed trailer read block to use `readttbit()` in new scanners (Luigi)
- New: Added support for "Fast Evil" loader, found in "The Evil Dead" (Luigi)

v0.22
-----

- Fix: Added proper SEQ file support (Luigi)
- Code quality: first attempt to make the source 64-bit safe in gcc compiler (Luigi)
- Code quality: removed system calls after suggestion from iAN CooG (Luigi)
- Improvement: moved prg file making from `analyze()` to `report()` (Luigi)
- New: Added support for "Action Replay" and "Easytape" loaders, developed by iAN CooG (Fabbo)
- Improvement: "Pavload" scanner improved, but cleaning of tapes won't yet fix a problem with the last pulse, which is known to be 0x3F if broken and no 0x1F preceeds it, otherwise it's 0x1F (Luigi)
- Fix: compilation fixes for Microsoft Visual C++ (Windows) and DJGPP (MS-DOS) (Fabbo)
- Code quality: uniformed code and comments in the new scanners (Luigi)
- Code quality: added read error check for unreadable checkbytes in the new scanners (Luigi)
- Code quality: removed "Easytape" and substituted "Action Replay" scanner (Luigi)
- Fix: fixed "prg" folder access rights upon creation under Linux (Luigi)
- New: Added support for "Ash+Dave" loader (Luigi)
- Fix: fixed pulse disambiguation check in c64tape.c (Fabbo)
- New: Added support for "Freeload Slowload" loader (Luigi)
- Fix: implemented integer wraparound prevention in new scanners (Luigi)
- Fix: fixed recognition of "CBM DATA" blocks when data starts with 0x01...0x06, by requesting a plausibility check on load and end addresses. Those blocks should not be misrecognized as "CBM HEADER" ones anymore. "Maggotmania" is now recognized properly (Luigi)
- Fix: fixed recognition of "CBM HEADER" blocks in "Ping Pong" (payload is 294 byte long) (Luigi)

v0.21
-----

- Fix: Fixed a compilation warning showing up in gcc 4.1.2 (Luigi)
- Fix: Fixed the trailer read code in turbotape.c (Luigi)
- Improvement: Tape type now affects CBM loader pulsewidths. `cps` as defined in TAP format (Luigi)
- New: Added support for "108DE0A5" loader (Luigi)
- Fix: Fixed all compilation warnings (Luigi)
- New: Added support for "Biturbo" loader (Luigi)
- New: Added support for reading DC2N 16-bit 2MHz RAW TAP format (Luigi+bgk)
- Code quality: converted scanners/ikloader.c to new coding style (bgk)
- Code quality: converted scanners/hitload.c to new coding style (bgk)
- Code quality: converted scanners/aces.c to new coding style (bgk)
- Fix: the DC2N to TAP conversion routines were fixed and improved a bit. This fixed the TAP time reporting bug (Luigi)

v0.20
-----

- Fixed the eof value for Gauntlet after a successful `addblockdef()` call (Luigi)
- Converted scanners/c64tape.c to new coding style (bgk)
- The Excluded Scanners message is only shown once (Luigi)
- Fix some compiler complaints (Luigi)
- Added Tap time to the report (bgk)
- Fixed the wav code. Now creating wavs should work. Thanks to Enthusi for discovering this bug (bgk+enthusi)

v0.19
-----

- Fixed off by one for tolerance. Now S.D.I. cleans 100% (Cyberload) with tol=15 (bgk)

v0.18
-----

- Fixed bug in "Mastertronic Burner Variant" (Luigi)
- Peepo/Luigi discovered a pilot size of only 96 bytes (was 192) (Luigi+Peepo)

v0.17
-----

- Updated loaders: cult, burner_var, oceannew4, accolade, rainbowarts_f1, rainbowarts_f2, tdi_f2 and trilogic (Luigi)
- New version of "ODEload" loader (Luigi)

v0.16
-----

- Changed the trailer ack routines for the scanners I created. I ack pulses up to a max amount, based on experience (Luigi)
- PARTLY Fixed the c64tape.c trailer read routine. Trailer is now accounted even if terminated by a spike, not just a pause (Luigi)
- Moved TDI F2 search before TDI F1 (Luigi)

v0.15
-----

- Edited "Cult" scanner (Luigi)
- Changed "TDI" scanner to acknowledge trailer pulses. All "TDI" TAP files which were optimized should be re-optimized (Luigi)
- Added scanner for "TDI F2". The discovery of EOF markers in "TDI" TAP files was fundamental (Luigi)
- Display the scanners you've elected to not run (bgk)
- Display list of scanners and -no<loader> parameters (bgk)

v0.14
-----

- Added 2 scanners: "Burner (Mastertronic Variant)" and "Ocean New 4" (Luigi)

v0.13
-----

- Added 3 scanners: "Rainbow Arts F1", "R.A. F2", and "Trilogic" (Luigi)

v0.12
-----

- Added scanner for "Alternative World Games" (fabbo)

v0.11
-----

- Additional fix inside "Audiogenic" scanner (Luigi)
- Fixed "Bleepload" scanner. The backtracing of pre-pilot pulses was missing a call to `is_pause_param()`. Mad Nurse is now 100% recognized after cleaning (Luigi)
- Final revision of the "Accolade/EA (+clone)" scanner (Luigi)
- Fixed `is_pause_param()` function in main.c The default return value did not work correctly at the very beginning of a TAP file. Boxing Manager is now 100% recognized after cleaning (Luigi)
- Some fixes to "Alien Syndrome" scanner (fabbo)

v0.10
-----

- Fixed "Audiogenic" scanner. The 0x01 byte after a block is NOT mandatory. This assumption was causing problems with "Loopz" (Luigi)
- Fixed the 'trace eof' and 'addblockdef' issues in `accolade_search()`. The same should be done for all similiar scanners (Luigi)
- Removed two levels of indentation inside `accolade_search()` (Luigi)

v0.09
-----

- Added "Accolade/EA (+clone)" support (Luigi)
- Added options for selecting c16, vic20 and c64 tapes. No actual code support for c16 and vic20 YET. (bgk)

v0.08
-----

- batchscan.c: Removed `static const char tcbatchreportname[] = "tcbatch.txt";` for it's already defined in main.c (Luigi)
- database.c/.h: I moved here the prg-related functions (`make_prgs()` and `save_prgs()`) from main.c and added `reset_prg_database()` to remove code duplication in `main()` and `save_prgs()` (Luigi)
- main.c: I changed many variables to static, improved the code within `get_exedir()` function Improved code in `main()` too: the variables now have the minimum required scope (Luigi)
- mydefs.h: this file reflects the above changes and contains the new OSAPI_xxx definitions which avoid the use of `#ifdef WIN32` inside the whole code where `system()` is called (Luigi)
- Expanded `display_usage()` (bgk)
- Added loader_id for "Cult" and removed for "Alien Syndrome" (bgk+Luigi)

v0.07
-----

- Converted scanners/usgold.c to new coding style (bgk)
- Updated Makefiles (bgk)
- Added support for "Alien Syndrome" loader (fabbo)
- Updated Makefile (fabbo)

v0.06
-----

Files changed: batchscan.c, database.c, database.h, main.c, mydefs.h, tap2audio.h, tap2audio.c

- Reinserted `temptcbatchreportname` definition in main.c. It proves to be an useful indicator while the bach scan is in progress. Declaration is _still_ in mydefs.h, it will be moved at a later time (Luigi)
- Moved au and wav filename definitions to the tap2audio.c file. Declaration is in tap2audio.h (Luigi)
- Removed `drawwavesquare()`, `drawwavesine()`, and `s_out()` prototypes from tap2audio.h, since those (PRIVATE) functions are not used outside the tap2audio.c file, and therefore they were made static there (Luigi)
- Added 1 level of indentation in `main()` for the TAP test (Luigi)
- Removed duplication of forward declarations of `addblockdef()` and `sort_blocks()` in main.h (Luigi)
- Changed many (PRIVATE) functions in main.c to static and commented out the declarations from main.h (Luigi)
- Added `#define HASNOTCHECKSUM -2` to database.h, thus eliminating the hardcoded value (Luigi)
- Moved function `scan_gaps()` to the database module (Luigi)
- Moved function `count_rpulses()` to the database module (Luigi)
- Moved function `count_good_checksums()` to the database module (Luigi)
- Moved function `compute_overall_crc()` to the database module (Luigi)

v0.05
-----

Replace hardcoded values with `#define`-s for `read_errors[]` (bgk)
Added check for `getcwd()` returning `NULL` (bgk)
Converted scanners/chr.c to new coding style (bgk)
Converted scanners/pause.c to new coding style (bgk)
Converted scanners/jetload.c to new coding style (bgk)
Converted scanners/freeload.c to new coding style (bgk)
Converted scanners/audiogenic.c to new coding style (bgk)
Some small cleanups in main.c (bgk)
Some small cleanups in clean.c (bgk)
Some small cleanups in batchscan.c. Also removed 1 level of indentation. (bgk)
Some small cleanups in filesearch.c (bgk)
Removed global `FILE *fp` from tap2audio.c (bgk)
Removed global `unsigned long bpos` from tap2audio.c (bgk)
Removed last global `char *outbuf` from tap2audio.c (bgk)
Removed 1 level of indentation in `main()` (bgk)
Converted scanners/hitec.c to new coding style (bgk)
Converted scanners/microload.c to new coding style (bgk)
Some small cleanups in scanners/microload.c (bgk)
Some small cleanups in scanners/hitec.c, freeload.c, jet.c (bgk)
Some small cleanups in scanners/enigma.c, oceannew1t1.c (bgk)

v0.04
Files changed: batchscan.c, database.c, main.c, mydefs.h

Converted `opname` in `main()` to a `char *`. No memory usage and no `strcpy()` anymore (Luigi)
Fixed `opnames[12][32]` that contains just 11 entries by converting it to `opnames[][32]` (Luigi)
Removed duplicated initialization of `tol` and `debug` inside `main()`. They are already initialized outside `main()` (Luigi)
Fixed `create_database()` call in `main()` which was not checking for the return value (Luigi)
Added a freeing of allocated resources in `create_database()` in case of `malloc()` failure (Luigi)
Fixed `wav_write()` call in `main()` which was called using the au filename (Luigi)
Removed usage of `temptcreportname`. Probably just useful in the GUI version of Final Tap (Luigi)
Converted `tcreportname`, `tcbatchreportname`, `temptcbatchreportname`, `tcinfoname`, `auoutname`, and `wavoutname` in main.c to static const char (Luigi)
Converted `cleanedtapname` in main.c to `static char` (Luigi)
Removed `tcreportname`, `tcbatchreportname`, `temptcbatchreportname`, `tcinfoname`, `auoutname`, `wavoutname`, and `cleanedtapname` from mydefs.h. Just `exedir` (non-static) was kept (Luigi)
Removed duplicated `BLKMAX`, `DBERR`, and `DBFULL` definitions from mydefs.h (Luigi)
Added `MAXPATH` definition in mydefs.h for strings holding path and some filenames (`exedir`, and `tap_t`/`tap_tr` subfields). Uniform to this definition elsewhere too or use dynamic strings? (Luigi)
Changed `strcmp` to `stricmp` in `get_exedir()` to avoid issues under Windows when using TAB from command line with the -b option (ie. "Tapclean -b" did not work) (Luigi)
Changed the '\\', "\\", '/', "/" in `get_exedir()` to use the SLASH #definition (Luigi)
Fixed drag&drop feature in `main()`. If you use "tapclean -b", `load_tap()` doesn't complain anymore (Luigi)
Partly Fixed the `getcwd()` call in `batchscan()` to make room for appending the `SLASH` and '\0'. See the TODO document (Luigi)
Fixed the remaining-taps-counter in `batchscan()` for batch scanning (Luigi)

v0.03
-----

Split database operations into database.[c|h] (Luigi+bgk)
Fixed some off by one errors in main.c (Luigi)
Added build info + builder (Luigi+bgk)
Some more cleanup (bgk)

v0.02
-----

Added "ODEload" (slc+bgk)
Added "Cult" loader (slc+bgk)
Added 'clean.' to start of cleaned tap filename (bgk)
Added -doprg parameter (bgk+peepo)
Some more cleanups. Most of the scanner/* files remain for now (bgk)
Fixed bug in `cbm_search()` (slc)

v0.01
-----

Cleanup of original FinalTAP console source (v2.76) (bgk)

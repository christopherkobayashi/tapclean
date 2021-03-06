TAPClean - TODO list
====================

Foreword
--------
Completed tasks are still included here as there's often a dependency on them within tasks not yet completed.
Some older tasks (at the bottom) might not be applicable any longer, but are kept here for reference.

- TODO: Allow users to specify the location of extracted PRG files (Mike)
- TODO: In "Gremlin GBH" checkbytes miss one pulse so are not yet handled: add support for partial bytes and reconstruction (Luigi)
- TODO: "Ocean New F3" and "Graphic Adv. Creator" use a different approach to cope with pilot pulses. Would it be possible to follow the same approach? (Luigi)
- TODO: "Ocean New F3" doesn't define a `tp` and uses `sp`, `mp`, `lp` in a quite unorthodox way. Perhaps "Graphic Adv. Creator" is slightly better as it defines a custom pilot finding function that could be made generic and used by "Ocean New F3" (Luigi)
- TODO: Add support for "Gyrospeed" - https://csdb.dk/release/?id=153648
- TODO: When cleaning the "Pavloda" title "Jet-Boot Jack" (English Software) the checkbyte is missing a few pulses so it fails to read properly, hence the count of `xtr` pulses fails to be set properly, and two bit 0 ($3F) pulses are not recognized hence clipped (TNT, Aki)

	- The pulse sequence in the last turbo file's checkbyte is wrong, and incomplete, so it fails to read properly in `pav_readbyte()`, hence the number of `xtr` pulses fails to be set properly. This means that two bit 0 ($3F) pulses are not recognized as part of the checksum/turbo file, hence clipped during the "Cutting post-data garbage" step in `cut_postdata_gaps()` (Luigi)
	- The checkbyte is unreadable for multiple reasons, so I'd say this title has to be repaired manually ahead of being cleaned (Luigi)
	- If I get to rewriting the scanner I will re-assess the best course of action, hoping to be able to do better regression testing by then (Luigi)

- TODO: Find a way to automatically handle split pulses in "Mega-Save" pre-pilots, as found in "Lethal" and "Soccer Manager", supplied by (Ziggy72)
- TODO: For all scanners, document pilot value and size within the source file. Same with sync and trailer. Started in microload.c and microload_var.c (Luigi)
- TODO: Work out which games use any given signature, i.e. first CBM Data CRC value (Luigi)
- TODO: Along with the `ft` array there should be a clear comment on how to choose `pmin`: The recommended value for `pmin` is 1/2 of the pilot size usually found on TAPs for very short pilot sequences (e.g. 8 bytes) and 3/4 of the pilot size for longer pilot sequences. Instead of `pmin` there could therefore be a `pavg` attribute representing the average amount of pilot pulses or bytes for each format and `pmin` could be worked out based on `pavg`, according to the given recommendation (Luigi)
- TODO: Consolidate the core processing in "Turbotape 250", "Turbotape 526", and "Turrican loader" as they use essentially the same file structure, 192-byte header file + data file (Luigi)
- TODO: Add an option to generate CRC-32 values using all possible bits of information within files on tape, in order to avoid cases like "Z" from  "Action Pack 2": the execution address within the last turbo file on  Side B (2404) was wrong (as it should have been 2304). However, as the  execution address is part of the file header, it is not included in the  file's CRC-32 calculation. Therefore, from an overall CRC-32 point, both sides looked equivalent (Luigi)
- TODO: There are two multi-title support approaches at the moment that should become the templates for multi-title support:

	- The one used in "Biturbo" and "Power Load": applicable to those cases when the encoding parameters are known (e.g. endianness, short pulse, long pulse, pilot/sync values) and it's just file details that are stored in the CBM part (RAM start address, end address, or file size); knowing encoding parameters we can cycle through each turbo file and then backtrace from there to hit each corresponding CBM file.
	- The one used in "Visiload": applicable to those cases when encoding parameters are set in the CBM part and need extracting on a per file basis, so that each turbo file in between CBM sections can be decoded; in this case we need to cycle through CBM parts, and then extract the encoding parameters required to detect turbo parts in between.

- TODO: Consider the unification of "Hi-Tech" and "Virgin" if they appear to be mastered by the same program with different settings. Also, there appear to be two Tx variants for "Virgin" ("Future Bike" thres=$016E, "Guardian II" thres=$015E) (Luigi)
- TODO: Titles that use "New Hi-Tec tape" really use a Tx variant of "Ocean New F2" so the latter should be extended with multiple Tx variants (Luigi)
- TODO: Refer to "additional bits" in "Visiload" as "stop bits" (Luigi)
- TODO: Allow a few more non-reserved characters in PRG filenames (Fredric)
- COMPLETED: Rewrite Bleepload scanners and make them usable with the "skewadapt" option as "bleep_f2.c" seems to struggle with it (Luigi)
- COMPLETED: The "Visiload" scanner should extract initial loader parameters from the CBM Data block as these were configurable at mastering time, as shown by early tapes. A similar change was done when rewriting "Rack-It" (Luigi)
- COMPLETED: A tape using multiple related "Visiload" titles and different initial loader parameters has been found: "Fun School". So the above change must extract loader parameters from the relevant CBM block instead of the first one on the tape (Paul Jones)
- TODO: "Visiload" tapes with level data on Side B are not yet supported as the scanner looks for CBM blocks to extract loader parameters, even when loader variables are preserved across scans. What can we do here? (Luigi)
- TODO: "Firebird" uses a very clever mechanism to avoid trying all known Tx variants: it looks for a pattern in the first CBM Header, which it then uses to extract the pulse read threshold. Once that's done, a single variant is searched for. Can we re-use this approach for e.g. "Alternative Software (DK) Tx" since T2 and T3 can be easily confused with the default read tolerance value? A similar approach could be used for "CHR", "Visi", "Ocean New", "Snakeload 5.0", and "Freeload Slowload". For backward compatibility: if no Tx pattern is found then the code should cycle through all Tx variants.
In fact, the above could be performed in `idloader()` when the CRC search fails, so that a `LID_<loader>_Tx` id could be returned and used as parameter to the `<loader>_search()` function, instructing it to only try the Tx variant instead of all variants.
In all cases, looking at the first CBM file only makes sense when a title is not part of a compilation: by using `cbm_header[]` / `cbm_program[]` directly in `idloader()` we lose room for future flexibility (Luigi)
- COMPLETED: There is just one reason for which "Ocean New" T1/T2 and "Snakeload 5.0" T1/T2 should not be handled by the same module, as it happens with all other scanners. The reason is that one might want to only exclude one type at a given time in order to avoid mis- or cross-detection. However, there is way too much code duplication unless both types are processed by the same scanner (Luigi)
- COMPLETED: "Ocean New" and "Snakeload 5.0" introduce a modular approach to support Tx variants: Roll it out to applicable scanners too, e.g. "Mega-Save" (Luigi)
- COMPLETED: "Snakeload 5.0" simplifies the sync sequence matching logic: Roll it out to applicable scanners too (Luigi)
- COMPLETED: Replace FILEIDOFFSET with BLKNUMOFFSET for consistency; consequently amend the comment "file ID offset inside header" (Luigi)
- COMPLETED: Rename GREMLIN_F1_search() to gremlin_f1_search() and so on (Luigi)
- COMPLETED: Titles using the "Alternative Software" loader were mastered by "The Graphic  Adventure Creator", in fact they are all text adventures with graphics (Ziggy72)
- COMPLETED: The loader known as "Alternative Software variant" and used by the "Graphic Adventure Creator" should be renamed to "Imagine/Incentive tape" and the loader used by games created with it, currently referred to as "Alternative Software tape", should be renamed to "Graphic Adv Creator tape" (Ziggy72)
- TODO: "U.S. Gold tape" was mastered with a tool called "Pro Cass" as found out by SLC who managed to get in touch with some ex-employees: the latter also confirmed that wrong checkbytes were embedded into some tapes in order to fool crackers. TAPClean could alert on this (Ziggy72)
- COMPLETED: "Cult tape" should be renamed to "Freeze Machine tape" (Ziggy72)
- COMPLETED: "FF Tape" stands for "Freeze Frame" (SLC) which later became "Freeze Machine" (Ziggy72)
- COMPLETED: "CHR" titles were mastered with a tool called "Mega-Save". In version 1.3, (C) 1984 Choice, each supported type is referred to as:

	- CHR T1 : Mega-Speed  x9 (fastest)
	- CHR T2 : Ultra-Speed x7 (medium)
	- CHR T3 : Hyper-Speed x5 (slowest)

The scanner should be renamed (Ziggy72)
- TODO: The code that starts with:
```c
find_decode_block(CBM_DATA, 1)
```
could be replaced with a lookup within the `cbm_program[]` array as it is populated with the contents of the very first CBM Data block during the search stage. Similarly the code that starts with:
```c
find_decode_block(CBM_HEAD, 1)
```
could be replaced with lookups within the `cbm_header[]`.
In all cases, looking at the first CBM file only makes sense when a title is not part of a compilation: by using `cbm_header[]` / `cbm_program[]` directly we lose room for future flexibility (Luigi)
- TODO: In clean_files() Shouldn't the following line:
```c
e = blk[i]->p3;		/* move block end backwards */
```
read as per below instead?
```c
e = blk[i]->p3+7;		/* move block end backwards */
```
The reason is that when `t == ACTIONREPLAY_STURBO` the trailer between p3+8 and p4 (inclusive) is cleaned using header pulses, so the rest goes from p1 to p3+7, where p3 the offset of first bit of the last data byte (Luigi)
- TODO: Avoid repetition for scanner code:

	- PRG data extraction and checkbyte calculation
	- pilot & trailer length computation (Luigi)

- TODO: Define a few MACROs for quicker access to data, e.g.:
```c
#define HEADER_GET_LOAD_ADDRESS() (hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8))
#define HEADER_GET_END_ADDRESS()  (hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8))
#define HEADER_GET_DATA_SIZE()    (hd[DATAOFFSETL] + (hd[DATAOFFSETH] << 8))
```
(Luigi)
- COMPLETED: The "Turrican" scanner reads the checksum as part of the data block (SLC)
- TODO: "Galactic Games" uses "Wildload" but pilot byte is 0x50 on level files (SLC)
- COMPLETED: "Barbarian II" uses a variant of the "Palace F1" loader with a different sync train. Also the first of the four sync bytes changes for every file, so it's a good idea to just match sync bytes 2-4 for identification. Same for the following "Martech" titles: "Brian Jack's Superstar Challenge" and "Eddie Kidd Jump Challenge" (SLC)
- TODO: The corrupted bit of post-data in "Digital Design Tape" should be reconstructed as its absence in a cleaned TAP might be causing load to fail in "Scooby Doo and Scrabby Doo" (Luigi)
- TODO: The size of the trailing silence should be overridable as "Hunchback at the Olympics" does not properly execute if the tape runs out before the unpacker has finished processing all data (Luigi - reported by Peepo and Ziggy72)
- COMPLETED: "Burner" uses a hardcoded execution address and a hardcoded count of files to load from tape, as opposite to its predecessor, the Mastertronic variant. However, "Hunchback at the Olympics" shows 15 bits post the data block, which might have been written by the masterer as dummy information that should at least be considered file trailer in order to be retained for historical reasons (Luigi)
- TODO: The use of MAXTRAILER should be replaced by the use of an extra attribute of the `fmt_t` structure, e.g. `tmax` (Luigi)
- TODO: readttbit() already checks for TAP data boundaries so the following:
```c
while (eof < tap.len - 1 &&
		h++ < MAXTRAILER &&
		readttbit(eof + 1, lp, sp, tp) >= 0)
	eof++;
```
can be simplified to (the order of the two conditions IS important):
```c
while (readttbit(eof + 1, lp, sp, tp) >= 0 && h++ < MAXTRAILER)
	eof++;
```
or to (the order of the two confitions is NOT important anymore):
```c
while (readttbit(eof + 1, lp, sp, tp) >= 0 && h < MAXTRAILER) {
	eof++;
	h++;
}
```
(Luigi)
- TODO: The generic sync sequence code, found e.g. in wildload.c, can be simplified so that it doesn't use an array of expected values but just calculates the n-th value based on the first sync byte and the n-th position, as it is done in chr.c (Luigi)
- TODO: The report is saved in a report buffer and then written to file. As the buffer has a fixed size, should we instead just write to file directly instead of buffering the report in memory? (Luigi)
- TODO: For CBM headers with payload > 192 bytes, the crc-32 is still computed on the first 192 bytes only. Is this acceptable? (Luigi)
- TODO: Make a consistent choice about the post-data patterns (in most cases these are described as part of the trailer, whose size is therefore augumented) (Luigi)
- TODO: Rewrite: burner.c (requires some decrypting), chr.c, firebird.c, freeload.c, hitec.c, hitload.c, ikloader.c, jetload.c, microload.c, oceannew1t*.c, oceannew2.c, rasterload.c, snake*.c, turbotape.c, usgold.c, virgin.c (also document the size of pilots/trailers in tapes using each of these) (Luigi)
- TODO: There doesn't appear to be any reason for abandoning search when nova_f1 block header contains broken bytes. Proceed with validation and further scanning (Luigi)
- IN PROGRESS: The block header validation recently (0.27) added to "Freeload" and "Ocean New" scanners by fabbo should be added to old scanners, wherever appropriate (Luigi)

	- Pending: superpav.c, supertape.c. Leave wildload.c as it is (chain of files without individual pilot+sync: if one is broken we have to give up)

- TODO: `describe_file(i)` is used in `analyze()` (via `describe_blocks()` in order to calculate the overall CRC-32 value that is displayed in the summary) and in report() (via print_database()) thus producing an unnecessary overhead (PRG contents are generated twice) while scanning.
`if (blk[row]->dd != NULL)` cannot always be used not to regenerate. We need the extra info and the error messages in the report that are produced while extracting PRG data (Luigi)

- TODO: Move WAV/AU file to same folder as source. (Luigi)
- TODO: Make sure the sof, sod, eod, eof values have a consistent meaning across all scanners. eod: it should always include postdata and/or checkbyte for consistency. See entry below suggested by SLC too. (Luigi)
- TODO: Trailer length: make sure postdata and/or checkbyte lengths are consistently subtracted (or not subtracted) from the trailing sequence length in all new scanners. See entry below suggested by SLC too. (Luigi)
- TODO: Allow partial EOFMARKER (i.e. only 7 valid pulses followed by a single over-stretched one) in tdi_f2.c. Last pulse is often corrupted. (Luigi)Code written, it is going to be tested.
- TODO: Review Hi-Tec and Virgin loader. These should really be Hi-Tec/Mastertronic T1, T2, T3, where:

	- CBM Data CRC32 0x895DCF44 -> T1 (Threshold 0x015E)
	- CBM Data CRC32 0x342A2416 -> T2 (Threshold 0x016E)
	- CBM Data CRC32 0xFADDF41C -> T3 (Threshold 0x017E)

Disambiguation should be achieved by means of the fast scanning based on CBM Data CRC32. (Luigi)
- TODO: When last pulse of a checkbyte is broken in Pavloda (either oversized or merged into a long pulse) the cleaning stage must fix it. The last pulse is 0x3F if no 0x1F preceeds it and it's 0x1F otherwise. (Luigi)
- TODO: It must be responsibility of the cleaning process to reconstruct the last bit of the last byte of pattern found in Ash+Dave trailer, 0x77, not of the scanner. Move the reconstruction there. (Luigi)
- COMPLETED: "Gremlin F2" should be split in T1 (Krakout, Bulldog) and T2 (Avenger). Possibly even more Tn variants each with its own threshold value. (Luigi)
- IN PROGRESS: Make some templates for the xxx_search() and xxx_describe() functions. (Luigi)
- IN PROGRESS: FIX xxx_search() functions by checking the return value from `addblockdef()` before assigning `eof` to `i`. (Luigi)
- TODO: Fix long / int usage in crc32.c. This is not a problem with gcc, but with MS Visual C++ that assumes that the size for int and long (4 bytes) are different
- IN PROGRESS: Check for memory and filehandle leaks (error handling).
- TODO: Add a portable strupr function for `sort_list()` in filesearch.c.
- TODO: Add warning about "Gremlin Fastload" (if we add that loader) (Luigi)

	- Only clean those if you know what you are doing.
	- Certain pulses can be misdetected/converted.

- TODO: Split the code into more manageable parts
- TODO: Make the code more generic
- TODO: Check start/end pilot/data/trailer. SLC says:

> Example from tcreport.txt: Location: $A726 -> $AF33 -> $75C9B -> $76491
The first points to the first pulse in the file (usually pilot).
The second as you know points to the first datapulse.
The third one points to the first pulse of the last byte..
this is usually a checkbyte, but in the case it's NOT,
it's pointing to the first pulse of the last databyte
This you also needs to compensate for when calculating the crc32 and
extracts data and also when counting trailers.
And the last value points to the very last pulse in a file, that being
last pulse of a databyte, checkbyte or just a trailer.
CHECK: ODEload + Cult
Probably check ALL loaders....

- TODO: Use dynamic memory allocation for file/dir names where possible.
- TODO: Avoid hardcoded values as much as possible, use #define.
- IN PROGRESS: Use "static" and "const" keywords whenever possible.
- TODO: Use the getcwd(buffer, size) call with buffer initialized to NULL and size to -1, which should be memory safe. Then allocate a new string whose size is buffer size + 2 to append SLASH and a '\0'. Strcpy and then free the first one.
- TODO: Do some major rewrite of scanners. There is WAY TOO MUCH duplication of code.
- TODO: Unify scanner/loader names/strings.
- TODO: Add new header info to all 'old' scanners and check that the code actually are doing what the header info says.
- TODO: Fix int - long problem with all ftell, fwrite, fseek etc functions
- TODO: Make it possible to create WAV without cleaning first...
- TODO: Fix broken CBM Header and Data Blocks if the other one of the pair is OK

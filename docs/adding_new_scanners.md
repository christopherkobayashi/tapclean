Adding new scanners to TAPClean
===============================
This is a cookbook intended to be used by FinalTAP and TAPClean scanner designers. It gives guidelines and code examples to follow when writing NEW scanners. To integrate your new scanner inside the above mentioned tools check the document about adding new scanners to FinalTAP (Stewart Wilson).

Definition of Terms
-------------------
*File*: plain data, ie. the information itself (a picture, a tune, a program, etc).
*Chunk*: the wrapped version of a file, as found on tapes. Usually this means a stream of pulses made up of a *lead-in sequence*, a *sync pattern*, usually a *header* section, a *data* section, and possibly a *trailer*.

Introduction
------------
To be aware of what's going on here, we need to make a step back and point out what loader designers had to bear in mind before writing their own loader.

Basically, to encode data on a sequential media, the following things have to be provided:
* a way to encode bits
* a way to recognize the start of a chunk of data while reading it in from tape
* a way to do a byte alignment while reading in a sequence of bits from tape

Usually, but not always, commercial tape loaders use a frequency shift keying (FSK) with just two frequencies. That is: bit 0 is encoded with a shorter duration (higher frequency) square wave and bit 1 with a longer duration (lower frequency) one.
To break down information into a stream of bits and sequentially write these to tape, it is necessary to choose if it's the **M**ost **S**ignificant **b**it (MSb) that has to be written first and then each subsequent one, up to the **L**east **S**ignificant **b**it (LSb), or the other way round. That's usually referred to as endianness, and therefore endianness is either **MSb F**irst (MSbF) or **LSb F**irst (LSbF).

Let's assume each piece of information (i.e. different files) has been broken down into different streams of bits (i.e. different chunks) and saved to tape. How do we know where each chunk starts? The main part of loaders use a pattern that tells them a new chunk is beginning. That pattern is known as *lead-in sequence*. It can be a sequence of the same byte value repeated quite many times, or the same bit value. As soon as that value changes into some known value (referred to as *sync pattern*), the loader is said to have done a complete synchronization with the stream coming from tape. In other words, the loader can be sure that what follows is the information that had been previously saved to tape.
The information (i.e. the files) can then be loaded into the computer memory for being used.

Usually the loader itself has to be loaded into the computer memory from tape, so there must be a built-in loader into the computer's ROM that can load a standard chunk from tape and execute it. In turn, the newly loaded code can load subsequent chunks using a different keying mechanism (that's why this code is referred to as "tape loader" or "turbo loader", the latter due to the fact that a custom loader is used to load data faster than the built-in loader).
The built-in loader is often referred to as "CBM tape loader" or "ROM Loader". It's the one loader that is executed when one types LOAD at the BASIC interpreter. It is beyond the scope of this document to illustrate how a turbo loader is stored inside a standard chunk and executed. If you are interested in that piece of information be sure to read the article about commercial turbo loaders.

Scanner Design
--------------
Bear in mind that a scanner is the product of reverse engineering of the turbo loader code **AND** inspection of the TAP file. You need to be proficient in 65xx ASM and CIA to do so. Again, check the article about commercial turbo loaders if you would like some help with that.

A FinalTAP or TAPClean scanner is composed of two sections: a *search* section and a *describe* section. The *search* section of each active scanner is run first and used to identify the chunks within the TAP file that use each supported loader. In this way, identified chunks can be correctly decoded (or *described*) at a later stage.

The *search* section attempts to recognize a turbo chunk by hunting for its known structure within the whole TAP file data: *lead-in sequence* + *sync pattern*, and size of the chunk based on the file length retrieved from the chunk header or from the standard chunk.

Once a chunk has been recognized, it has to be added to the internal database of recognized chunks by means of the function addblockdef(int lt, int sof, int sod, int eod, int eof, int xi). The meaning of those parameters is as per below:

* **lt** is chunk type, as declared in an enum in mydefs.h
* **sof** is the tape image offset of the first pulse that belongs to the chunk
* **sod** is the tape image offset of the first pulse that belongs to data section
* **eod** is the tape image offset of the pulse corresponding to the first bit of the last byte of the data section (that includes the data checksum, if any is present after data)
* **eof** is the tape image offset of the last pulse that belongs to the chunk. That is usually the last pulse of the trailer if there's one, otherwise it equals to eod
* **xi** is an extra information parameter, a 32bit value, used to pass information to the *describe* section.

More recently (May 2011) the function addblockdefex() has been added, which takes an extra parameter: addblockdefex(int lt, int sof, int sod, int eod, int eof, int xi, int meta1). The extra parameter is described below:
* **meta1** is used for additional information exchange between the *search* and *describe* stages where xi alone is not enough.

It is recommended **NOT** to set xi and meta1 to pointers for data allocated via malloc().

Different scenarios
-------------------
Now we can talk about the different scenarios that are likely to be found when reverse engineering one of those turbo loaders and looking at the distribution of values within the tape image.
What you should end up with, is a table of information like the following one specific for Accolade turbo loader:

**Threshold:** 0x01EA (490) **clock cycles** (TAP value: 0x3D)
**Bit 0 pulse:** 0×29 (average value)
**Bit 1 pulse:** 0x4A (average value)
**Endianness:** MSbF

**Pilot** byte: 0x0F (**amount of bytes:** 8)
**Sync** byte: 0xAA

**Header**
* 16 bytes: Filename<
* 02 bytes: Load address (LSBF)
* 02 bytes: Data size (LSBF)
* 01 byte : XOR Checksum of all Header bytes

**Data**
* Data is split in sub-blocks of 256 bytes each, or less for the last one.
* Each sub-block is followed by its XOR checksum byte. There are no pauses between sub-blocks.

**Trailer:** 8 Bit 0 pulses + 1 longer pulse.

The way we give FinalTAP and TAPClean the information contained above is by means of a fmt array entry. Based on the above table, the entry for this loader inside the fmt array is the following one:
```c
/* name,     en,   tp,   sp,   mp,  lp,   pv,   sv,   pmin, pmax, has_cs. */
{"ACCOLADE", MSbF, 0x3D, 0x29, NA,  0x4A, 0x0F, 0xAA, 4,    NA,   CSYES},
```
Where:
* **en** is endianness,
* **tp** is threshold (TAP value)
* **sp** is bit 0 pulse (or short pulse for those loaders that use 3 pulses to encode data)
* **mp** is med pulse (only significant for those turbo loaders that use 3 pulses to encode data)
* **lp** is bit 1 pulse (or long pulse for those loaders that use 3 pulses to encode data)
* **pv** is pilot (i.e. *lead-in*) value, it may be a byte or a bit
* **sv** is sync value, it may be a byte or a bit (note that this is just the first one in case there's a *sync pattern* made up of multiple bytes)
* **pmin** is the minimum amount of pilot bytes requested for a chunk to be identified during the *search* stage. The suggested value for pmin is 1/2 of the pilot size usually found on TAPs for very short pilot sequences (e.g. 8 bytes) and 3/4 of the pilot size for longer pilot sequences.
* **pmax** is the maximum number of pilot bytes to be used during the *search* stage. It is not usually used. Experienced designers can use this value to gather additional control over the*search* stage.
* **has_cs** is the flag with which we tell the program if the chunk in question has got a data checksum, so that it can give us correct stats about failed checksum checks.

Each *search* section uses common code, thanks to the definition of THISLOADER. This means that a designer can safely copy and paste code from an existing scanner to a new one, without being concerned about moving scanner specific information into the new scanner. Usually there's no need to change this convenient way to do things. If your new scanner is going to support more than one variant, please use a variable (eg. **variant**) and some emums to describe the variants. THISLOADER has been introduced to index the ft array only.
```c
#define THISLOADER	ACCOLADE

...

en = ft[THISLOADER].en;
tp = ft[THISLOADER].tp;
sp = ft[THISLOADER].sp;
lp = ft[THISLOADER].lp;
sv = ft[THISLOADER].sv;

...

for (i = 20; i > 0 && i < tap.len - BITSINABYTE; i++) {

	eop = find_pilot(i, THISLOADER);

	if (eop > 0) {

		/* Valid pilot found, mark start of file */
		sof = i;
		i = eop;

		/* Check if there's a valid sync byte for this loader */
		if (readttbyte(i, lp, sp, tp, en) != sv)
			continue;

		/* Valid sync found, mark start of data */
		sod = i + BITSINABYTE * SYNCSEQSIZE;

		...
```
Based on additional turbo chunk inspection, you should be able to provide the following information (the meaning of each field is discussed later on) in your C source code. This comes handy when you need to write a new scanner copying code from the new available scanners:
```c
/*
 * Status: Beta
 *
 * CBM inspection needed: No
 * Single on tape: No
 * Sync: Byte
 * Header: Yes
 * Data: Sub-blocks
 * Checksum: Yes (for each sub-block)
 * Post-data: No
 * Trailer: Yes
 * Trailer homogeneous: Yes (bit 0 pulses)
 */
```
That's it: if you need code for a new scanner that uses one sync byte, or that has a header, ot whose data is divided in sub blocks, just get the code from this file, both for the *search* and the *describe* sections.
Yes, it is really **THAT** easy, and it is the reason for which I designed the new scanners the way they are.

CBM inspection needed
---------------------
One option for tape loader designers was to store both the turbo loader code and a table with information about how many files to load from tape (and where in RAM to load them) inside the standard chunk. That's one approach, and it causes us serious headaches if the table is encrypted or placed in a point inside the standard chunk that changes on a per tape basis rather than being loader specific. I.e.: different tapes may use the same encoding scheme, the same loader code, but they may store that table in different locations.
The other (clever) option was to place information about each file inside the chunk, thus providing what's often called a chunk "header" (it may also be in a chunk of its own, of course). That header can contain, as example, the name of the data file that follows, where in RAM to load it, and how many bytes to load (or, equivalently, which is the location of the last byte to load). If each file has got its header, we don't have to bother seeking table entries inside the standard chunk.
We will refer to this different way to do things saying if "CBM inspection needed" is yes or no.

A way to pass information from the *search* to the *describe* routines in FinalTAP and TAPClean is to use the extended info field of the blk structure (the single unit of the file database). So that, once we extract information from the standard chunk during the *search* stage, we do not have to extract it again in the *describe* stage.
The extra info field is a 32bit integer in which we can pack two 16 bit values. Of course it is mainly intended to pack together load address and end address to pass to the *describe*function. Expert designers may find it useful to pack different information as well.

A snippet of code from cult.c follows:
```c
/* Store the info read from CBM part as extra-info */
xinfo = s + (e << 16);
```
A more complex scenario can be found in actionreplay.c, which shows this this field can be reused for different purposes:
```c
/* Pass details over to the describe stage */
xinfo = dt << 24; /* threshold */
xinfo |= (hd[CHKBYOFFSET]) << 16; /* checksum */
xinfo |= e; /* end address */
```

Single on tape
--------------
Let's assume we are unlucky: the turbo chunks do not contain any header. All the information is inside the standard chunk. If the turbo chunk is unique on tape, we may find the information about it inside the standard chunk and that's it. But what if there are more than one standard chunk on the tape each followed by a turbo chunk whose load details are in its respective standard chunk? In FinalTAP and TAPClean, standard chunk are recognized on the first scan process and acknowledged, so that it may be harder at a later time when searching for a turbo chunk to know which is the standard chunk for that chunk.
That's why we have to know if the turbo file is "Single on tape" or not.
An example of the worst scenario occurs with Biturbo: CBM inspection is needed because there are usually multiple files on the same tape (usually magazine tapes). A brilliant technique has been developed to process this case: we don't need to create new (buggy) code for that.

Sync
----
After the *lead-in sequence* has been read, a *sync pattern* is expected, so that if it's found the loader can reliably read in the following data, or even first read backwards and acknowledge a sequence of pre-pilot bytes (check the CHR scanner for an example).
If a *sync pattern* is not found, there's probably a disturb in the *lead-in sequence* that is not a *sync pattern*, not yet. Therefore the scanner (just as the loader code does itself) has to go back one step and try to read in the remaining part of the *lead-in sequence*. Eventually a *sync pattern* will be found.
The *sync pattern* can be just a bit (usually it's the other value than the one used by a *lead-in sequence* that consists in a sequence of the same bit value repeated), or a byte, or a sequence of bytes (ie. 2 or more bytes).
Since the code to read in those different *sync patterns* depends on the pattern itself, we have to specify if it is: a bit, a Byte, or a sequence (either of bits or Bytes). Then we can copy and paste the right code from an existing scanner to do the job.

One example that uses a pattern of bytes follows:
```c
#define SYNCSEQSIZE	17	/* amount of sync bytes */

...

/* Expected sync pattern */
static int sypat[SYNCSEQSIZE] = {
	0x10, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 
	0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01,
	0x00
};
int match;			/* condition variable */

...

/* Decode a byte sequence (possibly a valid sync train) */
for (h = 0; h < SYNCSEQSIZE; h++)
	pat[h] = readttbyte(i + (h * BITSINABYTE), lp, sp, tp, en);

/* Note: no need to check if readttbyte is returning -1, for 
         the following comparison (DONE ON ALL READ BYTES)
         will fail all the same in that case */

/* Check sync train. We may use the find_seq() facility too */
for (match = 1, h = 0; h < SYNCSEQSIZE; h++)
	if (pat[h] != sypat[h])
		match = 0;

/* Sync train doesn't match */
if (!match)
	continue;

/* Valid sync train found, mark start of data */
sod = i + BITSINABYTE * SYNCSEQSIZE;
```

Header
------
As I told before, turbo chunks can have a small piece of information that may contain the file name/ID, load address, data size/end address, and additional information. If there's a header, there's a common code to read it in and some define(s) to describe its structure (length and contents).
The presence of a header with load address and data size/end address may be crucial if there are multiple turbo files of the same type on one tape. If those files lack a header, it means the loader has a table of addresses that is used to load the turbo files in RAM. One part of this table may be inside the standard chunk, so that we can retrieve it easily, but other records may be anywhere in the following turbo files, in which case that information is too hard to retrieve.

One example of header being inside each chink is given below:
```c
#define HEADERSIZE	4	/* size of block header */

#define LOADOFFSETH	1	/* load location (MSB) offset header */
#define LOADOFFSETL	0	/* load location (LSB) offset header */
#define ENDOFFSETH	3	/* end location (MSB) offset inside header */
#define ENDOFFSETL	2	/* end location (LSB) offset inside header */

...

/* Read header */
for (h = 0; h < HEADERSIZE; h++) {
	hd[h] = readttbyte(sod + h * BITSINABYTE, lp, sp, tp, en);
	if (hd[h] == -1)
		break;
}
if (h != HEADERSIZE)
	continue;

/* Extract load and end locations */
s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
e = hd[ENDOFFSETL]  + (hd[ENDOFFSETH]  << 8);

// Prevent int wraparound when subtracting 1 from end location
if (e == 0)
	e = 0xFFFF;
else
	e--;

/* Plausibility check */
if (e < s)
	continue;
```
Another example is the following one:
```c
#define HEADERSIZE	5	/* size of block header */

#define FILEIDOFFSET	0	/* file ID offset inside header */
#define LOADOFFSETH	2	/* load location (MSB) offset inside header */
#define LOADOFFSETL	1	/* load location (LSB) offset inside header */
#define DATAOFFSETH	4	/* data size (MSB) offset inside header */
#define DATAOFFSETL	3	/* data size (LSB) offset inside header */

...

/* Read header */
for (h = 0; h < HEADERSIZE; h++) {
	hd[h] = readttbyte(sod + h * BITSINABYTE, lp, sp, tp, en);
	if (hd[h] == -1)
		break;
}
if (h != HEADERSIZE)
	continue;

/* Extract load location and size */
s = hd[LOADOFFSETL] + (hd[LOADOFFSETH] << 8);
x = hd[DATAOFFSETL] + (hd[DATAOFFSETH] << 8);

/* Compute C64 memory location of the _LAST loaded byte_ */
e = s + x - 1;

/* Plausibility check */
if (e > 0xFFFF)
	continue;
```

Data
----
Data is usually a continuous sequence of bytes but sometimes it was splitted into sub-blocks inside the same turbo chunk, separated by a checksum value for each sub-block. In the latter case there's usually a checksum byte every each 256 bytes of data. So that the *search* section must take into account the overload produced by those checksums that results in a data section inside the chunk longer than data size.
Turbo loaders that use sub-blocks, among the others: Accolade and Ocean new 4. An example of the overload calculation is provided below, from the accolade.c scanner.
```c
/* Compute size */
x = e - s + 1;

/* Compute size overload due to internal checksums */
bso = x / 256;
if (x % 256)
	bso++;

/* Point to the first pulse of the last checkbyte (that's final) */
/* Note: - 1 because "bso" also includes the last checkbyte! */
eod = sod + (HEADERSIZE + x + bso - 1) * BITSINABYTE;

/* Initially point to the last pulse of the last checkbyte */
eof = eod + BITSINABYTE - 1;
```

Checksum
--------
Some loader designers decided to protect data with one or more checksums, so that if the calculated checksum is not matching the expected one, probably a problem occurred while loading data from tape. If a load error occurred, the data cannot be reliably used. Some loaders just cause a soft reset if a load error occurred, other ones give the user the chance to try and reload the file from its beginning.
The presence of a checksum is crucial for once we find out all checksums in a tape match the expected values, we can almost surely say that data integrity has not been compromised at any point of the digitalization process nor by time.

Post-data
---------
Some turbo chunks have additional bytes just after the data section and they are not checksums. Sometimes they are just padding bytes. Sometimes they can be used to detect when the data chunk finished, which is useful in those cases different loaders use a similar data structure, but just one has got those additional bytes (example: TDI F2 and TDI F1).

Trailer
-------
Wise loader designers put some lead-out bytes just after the data section, to be sure data was properly read in. We usually know which is the total amount of trailer bytes, so it's good to check for that number at most. The reason is that sometimes there may be no evident separator between the trailer of one chunk and the *lead-in sequence* of the following chunk. On a real C64 that's not a problem, because the trailer is not read in and decoded. FinalTAP and TAPClean have to read it in to acknowledge it, so that we have to care about reading in the correct amount of trailer **pulses**.
```c
#define MAXTRAILER	8	/* max amount of trailer pulses read in */
```

Trailer homogeneous
-------------------
Some loaders use an homogeneous trailer, made up of bit 1 or bit 0 pulses, some others use just a combination of both.
Some examples follow.

Homogeneous trailer, made up of short pulses:
```c
/* Trace 'eof' to end of trailer (bit 0 pulses only) */
h = 0;
while (eof < tap.len - 1 &&
		h++ < MAXTRAILER &&
		readttbit(eof + 1, lp, sp, tp) == 0)
	eof++;
```
Non homogeneous trailer:
```c
/* Trace 'eof' to end of trailer (both bit 1 and bit 0 pulses) */
h = 0;
while (eof < tap.len - 1 &&
		h++ < MAXTRAILER &&
		readttbit(eof + 1, lp, sp, tp) >= 0)
	eof++;
```

Recommendations
---------------
Do not copy and paste code from the old scanners. If any feature you need in your own scanner is not available within the new scanners, just ask me to help with that. Old code is inconsistent and partly buggy.
New code is consistent and robust. Consistent means that the same thing is done always the same way, variables have always the same name, scope, and usage, so that it is easier to read and debug the new code. Robust means we learned from who came before us, while fixings their bugs, so that we got rid of those bugs in the new code.
If you end up with a scanner of your own by copying from old scanners, do NOT expect:
* me to help with issues that arise with it or to debug your code, and
* FinalTAP and TAPClean maintainers to insert it in the development trees

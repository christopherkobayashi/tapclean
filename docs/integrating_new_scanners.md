Integrating new scanners into TAPClean
======================================

- Write the new scanner .c file with the following functions:

	- `xxx_search()`
	- `xxx_describe()`
	- custom bit / byte reading functions, if `readttbit()` / `readttbyte()` are unsuitable

- Save your new format's source file in the 'scanners' folder and add the function prototypes to scanners.h.
- Add a definition for the format to the `ft[]` array (in main.c).
- Add a capitalized constant name for the format to the enum in mydefs.h, which must match the formats position in the `ft[]` array.
- Add a function call to your `xxx_search()` to the `search_tap()` function in main.c.
- Add a function call to your `xxx_describe()` to the `describe_file()` function in main.c.
- Add a global variable `noxxx` (allows disabling of the scanner) to main.c, and implement it as for all existing formats.
- Test thoroughly: be very careful that your newly added format does not conflict with any exisiting formats!

Adding loader IDs for quick scanning
------------------------------------
Add the loader recognition CRC(s) to the loader_id.c file, an entry in the mydefs.h enum of the form `LID_XXX`, and a format ID name in `knam` in main.c.
Add a function call to your `xxx_search()` to the `search_tap()` function in main.c, within the first section where loaders are ID-tested.

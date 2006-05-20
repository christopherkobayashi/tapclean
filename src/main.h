/*
 * main.h
 *
 * Part of project "Final TAP". 
 *
 * A Commodore 64 tape remastering and data extraction utility.
 *
 * (C) 2001-2006 Stewart Wilson, Subchrist Software.
 *
 *
 * This program is free software; you can redistribute it and/or modify it under 
 * the terms of the GNU General Public License as published by the Free Software 
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY 
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with 
 * this program; if not, write to the Free Software Foundation, Inc., 51 Franklin 
 * St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/* in main.c... */

void get_exedir(char *);
void display_usage(void);
void process_options(int, char **);
void search_tap(void);
void describe_file(int);
   
void describe_blocks(void);
int readttbit(int, int, int, int);
int readttbyte(int, int, int, int, int);
int find_pilot(int, int);
int load_tap(char *);
int save_tap(char *);
void unload_tap(void);
int analyze(void);
void report(void);
void clean(void);
int check_signature(void);
int check_version(void);
int check_size(void);
float get_duration(int, int);
int get_pulse_stats(void);
void get_file_stats(void);
int addblockdef(int, int, int, int, int, int);
void sort_blocks(void);
int count_bootparts(void);
void scan_gaps(void);
void gap_describe(int row);
int is_accounted(int);
int compute_overall_crc(void);
int count_rpulses(void);
int count_unopt_pulses(int slot);
int count_opt_files(void);
int count_good_checksums(void);
int count_pauses(void);
int is_pause_param(int);
void make_prgs(void);
int save_prgs(void);
void copy_loader_table(void);
void reset_loader_table(void);
void show_loader_table(void);
int find_decode_block(int, int);
int add_read_error(int);
void print_results(char *);
void print_database(char *);
void print_pulse_stats(char *);
void print_file_stats(char *);
   
/* general utilities... (could be put into their own file really) */
   
void msgout(char *);
int find_seq(int *, int, int *, int);    
void getfilename(char *, char *);
char* pet2text(char *, char *);
void trim_string(char *);
void padstring(char *, int);
void time2str(int, char *);
   
void deleteworkfiles(void);

/* in clean.c...  really an extension of main.c so no seperate header */

void fix_header_size(void);
void clip_ends(void);
void unify_pauses(void);
void clean_files(void);
void fix_boot_pilot(void);
void convert_to_v1(void);
void convert_to_v0(void);
void standardize_pauses(void);
void fix_pilots(void);
void fix_prepausegaps(void);
void fix_postpausegaps(void);
int insert_pauses(void);
void cut_range(int, int);
void cut_postdata_gaps(void);
void add_trailpause(void);
void fill_cbm_tone(void);
void fix_bleep_pilots(void);

/* in loader_id.c...  really an extension of main.c so no seperate header */

int idloader(unsigned long);

/* in batchscan.c... really an extension of main.c so no seperate header */

int batchscan(char *, int, int);

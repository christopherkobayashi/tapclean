/*
 * filesearch.h
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

#ifndef __FTFILESEARCH_H__
#define __FTFILESEARCH_H__

#define ROOTONLY	0
#define ROOTALL		1

struct node {
	char *name;
	struct node *link;
};

/**
 *	Prototypes
 */

struct node *filesearch_get_dir_list(char *);
struct node *filesearch_get_file_list(char *, struct node *, int);

int filesearch_free_list(struct node *);
int filesearch_sort_list(struct node *);
void filesearch_clip_list(struct node *);  

int filesearch_show_list(struct node *);
int filesearch_save_list(struct node *, char *);

#endif

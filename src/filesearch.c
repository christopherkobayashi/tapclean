/*
 * filesearch.c
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
 *
 * Notes:
 *  
 * Compiler: GCC
 *
 * The idea here is to have a program that can seek out and record ALL directory
 * names and file names (user masked) that exist above (inside and beyond) a specified
 * root directory.
 *
 * A linked-list is employed for the recording of names which should
 * make for a very safe and limitless data storage system with the ability
 * to be sorted quickly.
 *
 * Note: When opening files in a file list please remember to ignore
 *        the 1st entry, its name (path) is always the root directory name.
 *
 *
 * 16-apr-2006: Cleaned up by bokvamme@errors.no
 */

#include "filesearch.h"


/* Note: main is left here for usage information... */

/*
int main(int argc, char* argv[])
{
	struct node *dl, *fl;    
	char path[1024] = "D:\\Public\\MP3\\";	always include a trailing \\
	char mask[5] = "*.*";
	char exepath[1024];

	getcwd(exepath, 1024);

	printf("\npath = %s\n\nPlease wait...", path);

	dl = get_dir_list(path);
	if (dl != NULL)
		fl = get_file_list(mask,dl, ROOTALL);
	else
		return 1; 
      
	clip_list(fl);
   
	sort_list(dl);
	sort_list(fl);
     
	chdir(exepath);  
	save_list(dl, "_dirs.txt");    
	save_list(fl, "_files.txt");

	free_list(dl);
	free_list(fl);
	return 0;
} 
*/

/*
 * builds a linked-list of all directory names available under 'rootdir'...
 * returns a pointer to the linked-list of path names (root node).
 * Returns NULL on error.
 */

struct node *get_dir_list(char *rootdir)
{
	struct node *dirs, *c, *d;
	char cwd[1024], temp[1024];
	long handle;
	int done, complete, n, i;
#ifdef WINDOWS
	struct _finddata_t ffblk;
#else
	struct dirent **namelist;
#endif
    
	/* return NULL if rootdir doesnt exist */

	if (chdir(rootdir) == -1) {
		msgout("\nError: path does not exist");
		return NULL;
	}
      
	dirs = make_node(rootdir);	/* create root node (root DIR). */
	c = dirs;			/* current node is initially the root node. */
	d = dirs;			/* current directory node is initally the root node. */

	do {
		getcwd(cwd, 256);	/* record current directory path name.  */

		/*
		 * Find/record all directories in the current one (ignores "." and "..")
		 * a new node is created (and made current) for each one found...
		 */
#ifdef WINDOWS
		handle = _findfirst("*.*", &ffblk);
		if (handle != -1) {
			done = 0;            
			while (done != -1) {

				/* valid directory = a directory and not named '.' or '..' */  
            
				if (ffblk.attrib & _A_SUBDIR && strcmp(ffblk.name, ".") != 0 && strcmp(ffblk.name, "..") != 0) {

					/* create the name for the new node (CWD+\+DIR name)... */

					strcpy(temp, cwd);
					if (temp[strlen(temp) - 1] != SLASH) {
						temp[strlen(temp)] = SLASH;
						temp[strlen(temp) + 1] = '\0';
					}
					strcat(temp, ffblk.name);
                            
					/* create and switch to latest node... */

					c->link = make_node(temp);
					if (c->link == NULL)
						return NULL;	/* error creating node!. */
					c = c->link;
				}
				done = _findnext(handle, &ffblk);
			}   
		}
		_findclose(handle);
#else
		n = scandir(".", &namelist, 0, alphasort);

		for (i = 0; i < n; i++) {
			if (namelist[n]->d_type == DT_DIR &&
				strcmp(namelist[n]->d_name, ".") != 0 &&
				strcmp(namelist[n]->d_name, "..") != 0) {

				/* create the name for the new node (CWD+\+DIR name)... */

				strcpy(temp, cwd);
				if (temp[strlen(temp) - 1] != SLASH) {
					temp[strlen(temp)] = SLASH;
					temp[strlen(temp) + 1] = '\0';
				}
				strcat(temp, namelist[n]->d_name);
                            
				/* create and switch to latest node... */

				c->link = make_node(temp);
				if (c->link == NULL)
					return NULL;	/* error creating node!. */
				c = c->link;
			}
			free(namelist[n]);
		}
		free(namelist);
#endif
		/* switch to next available directory... */

		if (d->link != NULL) {
			chdir((d->link)->name);
			d = d->link;
			complete = 0;
		} else
			complete = 1;	/* we're finished. */

	} while(!complete);
  
	return dirs;
}

/*
 * Search all directories in linked list 'dirs' for files and build a new
 * list of all file names available in those dirs.
 * 'mask' provides the search parameter, ie.. "*.tap"
 * returns a pointer to the linked-list of path names (root node).
 */

struct node *get_file_list(char *mask, struct node *dirs, int searchtype)
{
	struct node *files, *d, *f;
	char temp[1024];
	long handle;
	int done, n, i;
#ifdef WINDOWS
	struct _finddata_t ffblk;
#else
	struct dirent **namelist;
#endif

	if (dirs == NULL)
		return NULL;

	/* create root node. (name= root directory name) */

	files = make_node(dirs->name);
	f = files;
	d = dirs;
   
	/* for each directory in the list... */

	while(d!=NULL) {
		chdir(d->name);
#ifdef WINDOWS
		handle = _findfirst(mask, &ffblk);
      
		if (handle != -1) {
			done = 0;
		
			while (done != -1) {

				/* valid file = NOT a directory! */ 

				if ((ffblk.attrib & _A_SUBDIR) == 0) {  
					strcpy(temp, d->name);
					if (temp[strlen(temp) - 1] != SLASH) {
						temp[strlen(temp)] = SLASH;
						temp[strlen(temp) + 1] = '\0';
					}
					strcat(temp, ffblk.name);
                        
					f->link = make_node(temp);
					f = f->link;
				}
				done = _findnext(handle, &ffblk);
			}
		}
		_findclose(handle);
#else
		n = scandir(".", &namelist, 0, alphasort);

		for (i = 0; i < n; i++) {
			if (namelist[n]->d_type == DT_REG) {
				strcpy(temp, d->name);
				if (temp[strlen(temp) - 1] != SLASH) {
					temp[strlen(temp)] = SLASH;
					temp[strlen(temp) + 1] = '\0';
				}
				strcat(temp, namelist[n]->d_name);

				f->link = make_node(temp);
				f = f->link;
			}
			free(namelist[n]);
		}
		free(namelist);
#endif
		d = d->link;	/* move to next dir in list. */
		if (searchtype == ROOTONLY)
			break;
	}

	return files;
}

/*
 * creates a new node pointer, allocates its memory and initializes it.
 * returns a pointer to the new node, it is the job of the calling function to
 * connect the new node to a linked-list.
 */

struct node* make_node(const char *name)
{
	struct node *n;
	
	n = (struct node*)malloc(sizeof(struct node));
	n->name = (char*)malloc(strlen(name) + 1);
	strcpy(n->name, name);
	n->link = NULL;
	
	return n;
}

/*
 * display the current state of the list onscreen...
 */

int show_list(struct node *r)
{
	struct node *c;
	int totaldirs = 0;

	c = r;
	do {
		printf("\n%p %p %s", c, c->link, c->name);
		printf("\n%s", c->name);
		c = c->link;
		totaldirs++;
	} while(c != NULL);

	/* -1 coz root is not included */

	printf("\n\nTotal directories : %d", totaldirs - 1);

	return 0;
}

/*
 * save the node list to file...
 * r is the root node for a linked list of nodes.
 * fname is the name of the file to create/write to.
 * returns 0 on success else 1.
 */

int save_list(struct node *r, char *fname)
{
	FILE *fp;
	struct node *c;
	int total = 0;

	fp = fopen(fname, "w+t");
	if (fp == NULL) {
		msgout("\n\nError: save_list(): failed to create file.");
		return 1;
	}
	
	c = r;
	while(c != NULL) {
		/* fprintf(fp,"\n%p %p %s", c,c->link,c->name); */
		fprintf(fp, "\n%s", c->name);
		c = c->link;
		total++;
	}

	fprintf(fp,"\n\nTotal entries %d", total - 1);  /* -1 coz root is not included  */
	fclose(fp);

	return 0;
}

/*
 *Frees memory for all nodes in a list starting with root node 'r'.
 */

int free_list(struct node *r)
{
	struct node *c, *t;

	c = r;
	while(c->link != NULL) {
		t = c;
		c = c->link;
		free(t->name);
		free(t);
	}

	free(c->name);
	free(c);

	return 0;
}

/*
 * Sort a linked-list (root node = r) by node 'name' property (Aa-Zz).
 * note: the list is sorted by the full PATH not just filenames.
 */

int sort_list(struct node *r)
{
	int swaps;
	struct node *n, *l1, *l2, *l3;
	char s1[1024], s2[1024], tmp[1024];
     
#define ND1 n		/* virtual nodes. be sure to affect 'n' (N1) LAST!  */
#define ND2 n->link
#define ND3 n->link->link

	if (r->link == NULL)	/* list is empty, do nothing. */
		return 0;
   
	do {
		n = r;
		swaps = 0;
		while(ND3 != NULL) {
			strcpy(s1, ND2->name);
			strcpy(s2, ND3->name);
//			strupr(s1);		/* make sort case insensitive... */
//			strupr(s2);		/* without this 'Zxxx' would appear before 'axxx' */
                
			if (strcmp(s1, s2) > 0) {

				/* swap N2 and N3...  */

				l1 = ND1->link;	/* save 3 links */
				l2 = ND2->link;
				l3 = ND3->link;
				ND3->link = l1;	/* remap nodes... */
				ND2->link = l3;
				ND1->link = l2;
				swaps++;
			}
			n = n->link;	/* step to next node */
		}
	} while(swaps != 0);

	return 0;
}

/*
 * Clip the root path (1st nodes name from all other nodes in the list and
 * replace with ".\" to make the paths relative.
 * This is really for the benefit of the sort routine as it causes all
 * directories to be placed at the top.
 * Note: the .\ prefixes could be removed after sorting.
 */

int clip_list(struct node *r)
{
   
	int i, k;
	struct node *n;
	char s[1024], tmp[1024];
	char rootname[1024];

	/* clip root paths and mark subdirectoried entries with a preceeding '.' */
	/* so they sort to the top (done with copies not the original entries). */ 
       
	/* directory root is held in name of 1st node */

	strcpy(rootname, r->name);
	k = strlen(rootname);        
   
	n = r->link;		/* begin with 2nd node */
                
	while(n != NULL) {
		strcpy(s, n->name);    
    
		for (i = 0; i < strlen(s); i++)  
			tmp[i] = s[k + i];     
		strcpy(s, tmp);   
      
		for (i = 0; i < strlen(s); i++) {

			/* if remainder of name s1 contains a \ its a subdir... */ 

			if (s[i] == SLASH) {

				/* mark it as such with a preceeding .\ */

				sprintf(tmp, ".%c%s", SLASH, s);
				strcpy(s, tmp);
				break;
			}    
		}          
		strcpy(n->name, s);
		n = n->link;		/* step to next node */
	}
}   


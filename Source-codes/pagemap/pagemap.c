/*
 * Parsing /proc/$pid/maps and for each address, parse /proc/$pid/pagemap to read the dirty-bit
 * We do this until the number of dirty bits becomes constant (we fix the treshold to 10).
 * Call the program with the pid of the process : 
 *  ./pagemap pid
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h> 

#define FILE_BUF 25
#define PAGE_SIZE 0x1000
#define TRESHOLD 10
#define INTERVAL 30


int parse_proc_pagemap(int pid, unsigned long a_start, unsigned long a_end)
{
    char pagemap_path[FILE_BUF];
    int pagemap_file_fd, nb_dirty = 0;

    snprintf(pagemap_path, FILE_BUF, "/proc/%d/pagemap", pid);

    /* On ouvre les fichiers */
    pagemap_file_fd = open(pagemap_path, O_RDONLY);
    if (pagemap_file_fd < 0) 
    {
		perror("fopen");
        exit(EXIT_FAILURE);
	}

    /* Pour chacune des adresses virt. dans la plage start-end, nous lisons l'adresse physique correspondante dans pagemap */
    for(uint64_t page = a_start; page < a_end; page += PAGE_SIZE) {
        uint64_t data, offset = (page / PAGE_SIZE) * sizeof(data); 

        if(pread(pagemap_file_fd, &data, sizeof(data), offset) != sizeof(data))
        {
            perror("pread");
            exit(EXIT_FAILURE);
        }
	
	 printf("0x%-16lx : pfn %ld soft-dirty %ld file/shared %ld "
		"swapped %ld present %ld\n",
		page,
		data & 0x7fffffffffffff,
		(data >> 55) & 1,
		(data >> 61) & 1,
		(data >> 62) & 1,
		(data >> 63) & 1);

        if((data >> 55) & 1) //bit 55 is the dirty bit
            nb_dirty++;
    }

    close(pagemap_file_fd);

    return nb_dirty;
}

int parse_proc_maps(int pid)
{
    char maps_path[FILE_BUF];
    char *line_buf = NULL, *line_buf_dup, *addr_start, *addr_end;
    FILE *maps_file;
    size_t size_line_buf = 0;
    ssize_t size_line;
    unsigned int nb_dirty = 0;

    /* On construit les chemins vers le fichier maps */
    snprintf(maps_path, FILE_BUF, "/proc/%d/maps", pid);

    /* On ouvre le fichier */
    maps_file = fopen(maps_path, "r");
    if (maps_file == NULL) {
		perror("fopen");
        exit(EXIT_FAILURE);
	}

    /* On parcourt /proc/maps ligne par ligne */
    while( (size_line = getline(&line_buf, &size_line_buf, maps_file)) != -1 )
    {
        uint64_t start, end;
        /* 
         * Chaque début de ligne est sous la forme addr_start-addr_end rw ...
         * donc on essaie de récupérer addr_start et addr_end 
         */
        line_buf_dup = strtok(line_buf, " "); //line_buf_dup points to the first string resulting on the split of the line
        addr_start = strtok(line_buf_dup, "-"); //now we're spliting the string "addr_start-addr_end"
        addr_end = strtok(NULL, "-");

        start = strtoul(addr_start, NULL, 16);
        end = strtoul(addr_end, NULL, 16);
        //printf("addr_start = %ld - addr_end = %ld\n", start, end);

        /* Maintenant nous devons parcourir pagemap pour chaque adresse dans la plage start-end */
        if(start > 0 && end > 0)
            nb_dirty += parse_proc_pagemap(pid, start, end);
    }

    free(line_buf);
    fclose(maps_file);

    return nb_dirty;
} 

int main (int agrc, char **argv)
{
    int pid = atoi(argv[1]), prev_dirty = 0, cur_dirty = 0;
    char clear_refs_path[FILE_BUF], *clear = "4";
    FILE *clear_refs_file;

    snprintf(clear_refs_path, FILE_BUF, "/proc/%d/clear_refs", pid);

    clear_refs_file = fopen(clear_refs_path, "w");
    if (clear_refs_file == NULL) {
		perror("fopen");
        exit(EXIT_FAILURE);
	}

    fwrite(clear, sizeof(clear), 1, clear_refs_file); //on efface tous les dirty-bit

    do{
        sleep(INTERVAL); //on attend 10secondes
        prev_dirty = cur_dirty; 
        cur_dirty = parse_proc_maps(pid); 
        printf("prev_dirty = %d - cur_dirty = %d\n", prev_dirty, cur_dirty);
        printf("diff = %d\n", cur_dirty - prev_dirty);
        fwrite(clear, sizeof(clear), 1, clear_refs_file);
    }while( (cur_dirty - prev_dirty) > TRESHOLD);

    return 0;
}

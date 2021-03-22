/*
 * Parsing /proc/$pid/maps and for each address, parse /proc/$pid/pagemap to read the dirty-bit
 * We do this the number of dirty bits becomes constant (we fix the treshold to 10).
 * Call the program with the pid of the process : 
 *  ./pagemap pid
 * 
 * Here we'll try to mmap the file instead of using getline to parse
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define FILE_BUF 25
#define PAGE_SIZE 0x1000
#define SEUIL 10
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
        addr_start = strtok(line_buf_dup, "-"); //now we spliting the sting "addr_start-addr_end"
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
    int clear_refs_fd;
    uint64_t *clear_ref_map;

    snprintf(clear_refs_path, FILE_BUF, "/proc/%d/clear_refs", pid);

    clear_refs_fd = open(clear_refs_path, O_RDWR);
    if (clear_refs_file < 0) {
		perror("open");
        exit(EXIT_FAILURE);
	}

    clear_ref_map = mmap(NULL, (1 << 6), PROT_WRITE, MAP_SHARED, clear_refs_fd, 0);
    if(clear_ref_map == MAP_FAILED){
        perror("mmap");
        close(clear_refs_fd);
        exit(EXIT_FAILURE);
    }

    clear_ref_map[0] = 4; printf("%ld\n", clear_ref_map[0]);

    do{
        sleep(INTERVAL); //on attend 10secondes
        prev_dirty = cur_dirty; 
        cur_dirty = parse_proc_maps(pid); 
        printf("prev_dirty = %d - cur_dirty = %d\n", prev_dirty, cur_dirty);
        printf("diff = %d\n", cur_dirty - prev_dirty);
        fwrite(clear, sizeof(clear), 1, clear_refs_file);
    }while( (cur_dirty - prev_dirty) > SEUIL);

    return 0;
}

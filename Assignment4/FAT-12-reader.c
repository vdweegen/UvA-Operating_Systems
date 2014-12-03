/* In this file a simple simulated FAT12 type file system is defined.
   The relevant structures in this file system will be:
   1) The disk information block BPB, describing the properties of the
      other structures.
   2) The FATs
   3) The root directory. This directory must be present.
   4) Data blocks
   All structures will be allocated in units of 512 bytes, corresponding
   to disk sectors.
   Data blocks are allocated in clusters of spc sectors. The FATs refer
   to clusters, not to sectors.
   The BPB below was derived from the information provided on the
   Atari TOS floppy format; it is also valid for MS DOS floppies,
   but Atari TOS allows more freedom in choosing various values.

   A start has been made to incorporate VFAT long file names (LFN),
   but this was not completed.
   Error checking is barely present - incorporating this is part
   of an OS assignment.

   G.D. van Albada
   (c) IvI, Universiteit van Amsterdam, 2012
   
*/
#include "FAT-12-reader.h"

/* The following are default values. Most are recomputed on basis of the 
   information in the bootblock.
*/
static int clusterSize = 1024;
static int bps = 512;
static int spc = 2;
static int dataStart = 0;

int fileAmount = 0;
int deletedAmount = 0;
int deletedFolders = 0;
int damaged = 0;
int broken = 0;
char name[80];

/* The file identified for the input file is stored here,
   as a global identifier
*/
static int fid;

/* Two macros to help convert bytes to short values and 
   to 4 byte integers
*/
#define toShort(b) ((b[0] & 0xff) + 256 * (b[1] & 0xff))
#define toLong(b) ((b[0] & 0xff) + (((long)(b[1] & 0xff)) << 8) +\
                  (((long)(b[2] & 0xff)) << 16) +\
                  (((long)(b[3] & 0xff)) << 24))


/* Give the user the ability to choose */
int doChoose(char text[80])
{
    printf("%s", text);
    int valid_choice = 0;
    char ch;
    while(valid_choice == 0)
    {
        scanf("%c", &ch );
        if((ch == 'Y') || (ch == 'N') || (ch == 'n') || (ch == 'y'))
        {
            valid_choice = 1;
        }
        if((ch == 'Y') || (ch == 'y'))
        {
            return(1);
        }
        else
        {
            return(-2);
        }
    }
    return(-2);
}

/* print a directory entry */
void printDirEntry(dirEntry * e)
{
    int i;
    if (e->attrib == 0x0f)
    {
        printf("LFN:");
        for (i = 1; i < 32; i++)
        {
            if (e->name[i] >= ' ')
            {
                printf("%c", e->name[i]);
            }
            else if (e->name[i])
            {
                printf("&%x;", e->name[i]);
            }
        }
        printf("\n");
    }
    else
    {
        /* Remove Clutter in Terminal */
        if(e->attrib == 0)
        {
            return;
        }
        
        /* Start Processing */
        for (i = 0; i < 8; i++)
        {
            if (e->name[i] < ' ')
            {
                break;
            }
            printf("%c", e->name[i]);
        }
        printf(".");
        for (i = 0; i < 3; i++)
        {
            if (e->ext[i] < ' ')
            {
                break;
            }
            printf("%c", e->ext[i]);
        }
        printf(" (%x)", e->attrib);
        for (i = 0; i < 10; i++)
        {
            printf("%2x", e->zero[i]);
        }
        printf(" time: %hu date: %hu start: %hu length: %ld\n",
            toShort(e->time),
            toShort(e->date), toShort(e->start), toLong(e->length));   
    }
}

/* The following code should obtain the total actual number of clusters
   in a chain starting at some directory-entry.start
*/
int followDirEntry(dirEntry *e, unsigned short * sFAT)
{
    int cur = toShort(e->start);
    int next;
    int nclusters = 0;
    int size = toLong(e->length);
    int nexpected = 0;
    nexpected = (size + clusterSize - 1) / clusterSize;
    printf("Following chain from %d\n", cur);
    /* For directories a length of zero is generally specified.
       This code will thus read only a single cluster for directories. */
    if (e->attrib == 0x0f)
    {
        return 0;
    }
    
    if (e->attrib & 0x08)
    {
        printf("Volume label, not a file\n");
        return 0;
    }
    
    if (cur < 2)
    {
        printf("Not a valid starting cluster\n");
        return 0;
    }
    
    do
    {
        nclusters++;
        next = sFAT[cur];
        /* Removed to De-Clutter */
        /* printf("%d ", next); */
        cur = next;
    } while (next && (next < 0x0FF0) && (nclusters < nexpected));
    printf("\nNclusters = %d\n", nclusters);
    return nclusters;
}

/* This routine will read a file from disk and store it in a buffer
*/
int bufferFile(dirEntry *e, unsigned short * sFAT, char ** buffer)
{
/* First find number of clusters in file */
    int cur = toShort(e->start);
    int nclusters = followDirEntry(e, sFAT);
    int nbytes;
    int nread = 0;
    int next;
    int offset = 0;
    (*buffer) = NULL;
    if (nclusters == 0) return 0;

    *buffer = calloc(nclusters * bps * spc, 1);
    do
    {
        lseek(fid, (cur + dataStart) * clusterSize, SEEK_SET);
        if (clusterSize !=
                (nbytes = read(fid, (*buffer) + offset, clusterSize)))
        {
            printf("Disk read error, expected %d bytes, read %d\n",
                   clusterSize, nbytes);
            printf("Attempting to read cluster %d\n", cur);
            return -1;
        }
        nread++;
        offset += clusterSize;
        next = sFAT[cur];
        cur = next;
    } while (next && (next < 0x0FF0) && (nread < nclusters));
    if (next < 0x0FF0)
    {
    /* not a normal end of chain */
        printf("Broken file, read %d clusters, expected %d,"
               "next cluster would be at %d\n",
                nread, nclusters, next);
        broken++;
        return(-2);
    }
    return nclusters;
}

int extractDirEntry(dirEntry *e, unsigned short * sFAT)
{
    int i;
    FILE *p = NULL;
    char file[80] = "Recovered_";
    
    /* Filter for LongFileNames */
    if (e->attrib == 0x0f)
    {
        printf("LFN Not supported (yet)\n");
    }
    else
    {
        for (i = 0; i < 8; i++)
        {
            if (e->name[i] <= ' ')
            {
                break;
            }
            sprintf(file, "%s%c", file, e->name[i]);
        }
        
        /* Not the most elegant solution, but it works */
        if(strcmp(file, "Recovered_") == 0)
        {
            return(-2);
        }
        
        /* See if an extension is present */
        if(e->ext[0] == ' ')
        {
            return(-2);
        }
    }
    
    /*
     * Check if a file is damaged (multiple checks)
     * First do a crude check, then do a thorough check.
     */
    if((toLong(e->length) % 100) == 0) /* Crude check, round file (which is highly unusual) */
    {
        damaged++;
        name[0] = '\0';
        sprintf(name, "The file '%s' is probably damaged.. Extract Anyway? (Y/N)\n", file);
        if(doChoose(name))
        {
            sprintf(file, "%s_CORRUPT", file);
        }
        else
        {
            return(-2);
        }
    }
    
    /* Add the extension */
    sprintf(file, "%s.", file);
    for(i = 0; i < 3; i++)
    {
        if (e->ext[i] <= ' ')
        {
            break;
        }
        sprintf(file, "%s%c", file, e->ext[i]);
    }
    
    /* free the buffer */
    char * buffer = NULL;
    free(buffer);
    
    /* Place desired file in buffer */
    bufferFile(e, sFAT, &buffer);
    
    /* Check if the file already exists */
    if(fopen(file, "r"))
    {
        printf("File '%s' already exists..\n", file);
        printf("\tSkipping file\n");
        return(-2);
    }
    
    /* See if we can write */
    p = fopen(file, "w");
    if(p == NULL)
    {
        return(-2);
    }
    
    /* Write buffer to the file */
    if(fwrite(buffer, toLong(e->length), 1, p))
    {
        /* Increment fileAmount counter */
        fileAmount++;
    }
    
    /* Close filestream */
    fclose(p);
    
    return(0);
}
/* Read the entries in a directory (recursively).
   Files are read in, allowing further processing if desired
*/
int readDirectory(dirEntry *dirs, int Nentries, unsigned short * sFAT)
{
    int i, j;
    char * buffer = NULL;
    int nclusters = 0;
    for (j = i = 0; i < Nentries; i = j + 1)
    {
        extractDirEntry(dirs + i, sFAT);
        printDirEntry(dirs + i);
        for (j = i; j < Nentries; j++)
        {
            if(dirs[j + 1].attrib != 0x0f)
            {
                /* Fix: Longfilename -> Parse */
                break;
            }
            printDirEntry(dirs + j);
        }
        
        if ((dirs[i].name[0] == 0x05) || (dirs[i].name[0] == 0xe5))
        {
            printf("Deleted entry (%d)\n", dirs[i].name[0]);
            if((dirs[i].attrib == 0x10))
            {
                printf("OHOH!... DELETED FOLDER!\n");
                deletedFolders++;
            }
            else
            {
                deletedAmount++;
            }
        }

        if (dirs[i].name[0] > ' ' && (dirs[i].name[0] != '.'))
        {
            free(buffer);
            nclusters = bufferFile(dirs + i, sFAT, &buffer);
            if (buffer && (dirs[i].attrib & 0x10) && (nclusters > 0))
            {
                int N;
                /* this must be another directory follow it now */
                N = nclusters * clusterSize / sizeof(dirEntry);
                printf("Reading directory\n");
                readDirectory((dirEntry *) buffer, N, sFAT);
            } 
        }
    }
    free(buffer);
    return 0;
}

/* Convert a 12 bit FAT to 16bit short integers
*/
void expandFAT(uint8_t * FAT, unsigned short * sFAT, int entries)
{
    int i;
    int j;
    for (i = 0, j = 0; i < entries; i += 2, j += 3)
    {
        sFAT[i] = FAT[j] + 256 * (FAT[j + 1] & 0xf);
        sFAT[i + 1] = ((FAT[j + 1] >> 4) & 0xf) + 16 * FAT[j + 2];
    }
}

/* Convert a FAT represented as 16 bit shorts back to 12 bits to
   allow rewriting the FAT
*/
void shrinkFAT(uint8_t * FAT, unsigned short * sFAT, int entries)
{
    int i;
    int j;
    for (i = 0, j = 0; i < entries; i += 2, j += 3)
    {
        FAT[j] = sFAT[i] & 0xff;
        FAT[j + 1] = ((sFAT[i] & 0x0f00) >> 8) + ((sFAT[i + 1] & 0x000f) << 4);
        FAT[j + 2] = (sFAT[i + 1] & 0x0ff0) >> 4;
    }
}

/* We'll allow for at most two FATs on a floppy, both as 12 bit values and
   also converted to 16 bit values
*/
uint8_t * FAT1;
uint8_t * FAT2;

unsigned short * sFAT1;
unsigned short * sFAT2;

int main(int argc, char * argv[])
{
    BPB bootsector;
    int rv = 0;
    int entries;
    int NFATbytes;
    int dataSectors;
    int clusters;
    int i;
    int Ndirs;
    int NdirSectors;
    int nread;
    dirEntry *dirs;

    /* Clear the terminal */
    system("clear");
    
    if (argc > 1)
    {
        fid = open(argv[1], O_RDONLY);
    } else
    {
        printf("\nUsage: ./reader <image>\n\n");
        exit(-1);
    }
    if ((sizeof(BPB) != (rv = read(fid, &bootsector, sizeof(BPB)))))
    {
        printf("Read error %d\n", rv);
        exit(-2);
    }
    printf("size of bootsector = %lu\n", sizeof(BPB));
    
    printf("Serial nr. %u-%u-%u\n", bootsector.vsn[0],
             bootsector.vsn[1], bootsector.vsn[2]);
    printf("bps = %hu\n", bps = toShort(bootsector.bps));
    printf("spc = %hu\n", spc = bootsector.spc);
    printf("res = %hu\n", toShort(bootsector.res));
    printf("NFats = %hu\n", bootsector.NFats);
    printf("Ndirs = %hu\n", Ndirs = toShort(bootsector.Ndirs));
    printf("Nsects = %hu\n", toShort(bootsector.Nsects));
    printf("Media = %hu\n", bootsector.Media);
    printf("spf = %hu\n", toShort(bootsector.spf));
    printf("spt = %hu\n", toShort(bootsector.spt));
    printf("Nsides = %hu\n", toShort(bootsector.Nsides));
    printf("Nhid = %hu\n", toShort(bootsector.Nhid));
    if (!bps || !bootsector.NFats  || !toShort(bootsector.Nsects))
    {
        printf("This is not a FAT-12-type floppy format\n");
        exit(-2);
    }
    /* Compute the number of entries in the FAT
       The number of bytes is BPS * spf
       The maximum number of entries is 2/3 of that.
       On the other hand, we have a maximum of x data-sectors,
       where x = Nsects - 1 - NFats * spf
       or mayby x = Nsects - res
    */
    NFATbytes = toShort(bootsector.bps) * toShort(bootsector.spf);
    entries = (2 * NFATbytes) / 3;
    dataSectors = toShort(bootsector.Nsects) - 1 /* boot */ -
                  bootsector.NFats * toShort(bootsector.spf);
    clusters = dataSectors / bootsector.spc;
    clusterSize = spc * bps;
    printf("entries = %u, dataSectors = %u, clusters = %u\n", entries,
           dataSectors, clusters);
    FAT1 = malloc(NFATbytes);
    FAT2 = malloc(NFATbytes);
    nread = read(fid, FAT1, NFATbytes);
    if (nread != NFATbytes)
    {
        printf("Unexpected EOF\n");
    }
    for (i = 1; i < bootsector.NFats; i++)
    {
        nread = read(fid, FAT2, NFATbytes);
        if (nread != NFATbytes)
        {
            printf("Unexpected EOF\n");
        }
    }
    sFAT1 = calloc(entries + 1, sizeof(unsigned short));
    sFAT2 = calloc(entries + 1, sizeof(unsigned short));
    expandFAT(FAT1, sFAT1, entries);
    expandFAT(FAT2, sFAT2, entries);
    printf("FAT1: %hu  %hu  %hu  %hu  %hu  %hu\n", sFAT1[0],
            sFAT1[1], sFAT1[2], sFAT1[3], sFAT1[4], sFAT1[5]);
    printf("FAT2: %hu  %hu  %hu  %hu  %hu  %hu\n", sFAT2[0],
            sFAT2[1], sFAT2[2], sFAT2[3], sFAT2[4], sFAT2[5]);
    
    i = bps / sizeof(dirEntry);
    NdirSectors = (Ndirs + i - 1) / i;
    dataStart = 1 + (bootsector.NFats * NFATbytes) / bps + NdirSectors - 2;
    dirs = calloc(Ndirs, sizeof(dirEntry));
    printf("dataStart = %d\n", dataStart);
    nread = read(fid, dirs, bps * NdirSectors);
    if (nread != bps * NdirSectors)
    {
        printf("Unexpected EOF\n");
    }
    readDirectory(dirs, Ndirs, sFAT1);
    
    /* Print our finale */
    printf("\nThe file '%s' has been succesfully extracted:\n", argv[1]);
    printf("\tThere were %d file(s) extracted\n", fileAmount);
    printf("\tThere were %d folder(s) marked as deleted\n", (deletedFolders / 2));
    printf("\tThere were %d file(s) marked as deleted\n", deletedAmount);
    printf("\tThere were %d damaged file(s)\n", damaged);
    if(broken > 0) printf("\tSome clusters of this image were broken\n");
    return 0;
}

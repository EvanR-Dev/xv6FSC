///////////////////////////////////////////////////////////////////
// Important info:
// Block 0 = unused
// Block 1 = super block
// block 2 to ... = inodes
// after = bitmap

// searches start from root dir: ROOTINO 1
// block size = 512
// superblock has: size, number of data blocks (nblocks),
// and number of inodes (ninodes)
// read superblock first to traverse for fschk

// inode struct:
// type = file or dir or device, in stat.h: T_DIR = 1, T_FILE = 2,T_DEV = 3
// IPB = inodes per block

// directory entry struct:
// inum
// names
///////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <sys/types.h>
//#include <sys/mman.h>
#include <sys/stat.h> // for use of fstat
//#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

//#include "types.h"
//#include "fs.h"

// prototypes
// helpers
void errorHandler(bool[], bool);
void rsect(uint, void);
uint xint(uint);

// checks
void fsChk1();
// add more with param
// ...

#define BLOCK_SIZE (BSIZE)

int main(int argc, char *argv[])
{
    int r, i, n, fsfd;
    char *addr;
    struct dinode *dip;
    struct superblock *sb;
    struct dirent *de;

    // other specifications: no image file is provided
    if (argc < 2)
    {
        fprintf(stderr, "Usage: fcheck <file_system_image>");
        exit(1);
    }

    // open file image to be chked
    fsfd = open(argv[1], O_RDONLY);
    // image file did not open
    if (fsfd < 0)
    {
        perror(argv[1]);
        exit(1);
    }

    ///////////////////////// ...Begin real changes... /////////////////////////////

    /* Dont hard code the size of file. Use fstat to get the size */
    // map file as part of memory (memory mapped I/O)
    struct stat sz;
    fstat(fsfd, &sz); // fstat on file desc
    addr = mmap(NULL, sz.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
    if (addr == MAP_FAILED)
    {
        perror("mmap failed");
        exit(1);
    }

    /* read the super block */
    // ptr to superblock for direct access
    sb = (struct superblock *)(addr + 1 * BLOCK_SIZE);
    // printf("fs size %d, no. of blocks %d, no. of inodes %d \n", sb->size, sb->nblocks, sb->ninodes);

    /* read the inodes */
    // ptr to inode
    dip = (struct dinode *)(addr + IBLOCK((uint)0) * BLOCK_SIZE);
    // printf("begin addr %p, begin inode %p , offset %d \n", addr, dip, (char *)dip -addr);

    // get the address of root dir
    de = (struct dirent *)(addr + (dip[ROOTINO].addrs[0]) * BLOCK_SIZE);

    ////////////////////////////// traverse fs ////////////////////////////////////////

    // bool arr where each idx, 1 to 12, represents an error if true
    bool chkFails[13] = {0};

    // 1
    bool isValidOrAllocated;

    // 3
    // inode 1 must be a dir
    if (dip[ROOTNO].type != 1)
    { // 3
        chkFails[3] = true;
        errorHandler(chkFails, false);
    }

    // traverse all entries within the init direct block as indicated by de
    int j = 0;
    while (j < BSIZE / sizeof(struct dirent))
    { // 3
        // verify parent dir -> ROOTINO
        if (strcmp(de->name, "..") == 0)
        {
            if (de->inum == ROOTINO)
            {
                return;
            }
            else
            {
                chkFails[3] = true;
                errorHandler(chkFails, false);
            }
        }
        j++;
        de++;
    }

    // at this point the parent DNE
    chkFails[3] = true;
    errorHandler(chkFails, false);

    // 6
    // iterate through bitmap (data blocks)
    int b = BBLOCK((sb->size), sb->ninodes) + 1;
    while (b != sb->size)
    {
        char *bitMap = (char *)(addr + BBLOCK(0, sb->ninodes) * BLOCK_SIZE);
        int x = 1 << (b % 8);
        if ((bitMap[b / 8] * b) != 0)
        {
            bool found = false;
            int i = 0;
            while (i < sb->ninodes)
            {
                // allocated
                if (dip[i].type != 0)
                {
                    int j = 0;

                    // dir blocks
                    while (j < NDIRECT)
                    {
                        if (dip[i].addrs[j] == b)
                        {
                            found = true;
                            break;
                        }
                        j++;
                    }

                    if (dip[i].addrs[NDIRECT] == b)
                    {
                        found = true;
                        break;
                    }

                    // check indir addrs
                    uint indirect[NINDIRECT];
                    rsect(xint(dip[i].addrs[NDIRECT]), (char *)indirect);
                    int k = 0;
                    while (k < NINDIRECT)
                    {
                        if (indirect[k++] == b)
                        {
                            found = true;
                            break;
                        }
                    }
                }
                i++;
            }

            if (!found)
            { // 6
                chkFails[6] = true;
                errorHandler(chkFails, true);
            }
        }
        b++;
    }

    ////////////////////////////////////////// main iteration /////////////////////////////////////////////

    // the current inode
    int i = 0;
    // iterate thru every inode
    while (i < sb->ninodes)
    {
        isValidOrAllocated = dip[i].type >= 0 && dip[i].type <= 3
        if (!isValidOrAllocated)
        { // 1
            chkFails[1] = true;
            errorHandler(chkFails, false);
        }

        // allocated inode
        if (dip[i].type != 0)
        {

            // checks: 9
            bool found = false;
            int ref = 0;
            // check ref in a directory
            while (ref < sb->ninodes)
            {
                if (dip[i].type == 1)
                {
                    int dirBlk = 0;
                    // entries in dir blocks
                    while (dirBlk < NDIRECT)
                    {
                        struct dirent *de = (struct dirent *)(addr + (dip[ref].addrs[dirBlk]) * BLOCK_SIZE);
                        int j = 0;
                        while (j < BSIZE / sizeof(struct dirent))
                        {
                            if (de->inum == i && strcmp(de->name, ".") != 0)
                            {
                                found = true;
                            }
                            j++;
                            de++;
                        }
                        dirBlk++;
                    }

                    if (!found)
                    {
                        uint indirect[NINDIRECT];
                        rsect(xint(dip[ref].addrs[NDIRECT]), (char *)indirect);

                        int indirBlk = 0;
                        while (indirBlk < NINDIRECT)
                        {
                            if (indirect[indirBlk] > 0)
                            {
                                struct dirent *de = (struct dirent *)(addr + (indirect[indirBlk]) * BLOCK_SIZE);
                                int idx = 0;
                                while (idx < BSIZE / sizeof(struct dirent))
                                { // 9
                                    if (de->inum == idx && strcmp(de->name, ".") != 0)
                                    {
                                        found = true;
                                    }
                                    idx++;
                                    de++;
                                }
                            }
                            indirBlk++;
                        }
                    }
                }
                ref++;
            }

            // 9
            if (!found)
            {
                chkFails[9] = true;
                errorHandler(chkFails, false);
            }

            // checks: 4, 9, 10
            // inode is a dir: T_DIR
            if (dip[i].type == T_DIR)
            {
                int dirBlk = 0;
                bool oneDot = false, twoDots = false;

                // direct block entries
                while (dirBlk < NDIRECT)
                {
                    struct dirent *de = (struct dirent *)(addr + (dip[i].addrs[dirBlk]) * BLOCK_SIZE);
                    int sz = dip[i].size / sizeof(struct dirent);
                    int k = 0;
                    while (k < sz)
                    {
                        // located .. or . entry
                        if (strcmp(de->name, ".") == 0)
                            oneDot = true;
                        if (strcmp(de->name, "..") == 0)
                        {
                            twoDots = true;

                            // verify . has correct definition
                            if (i != de->inum)
                            { // 4
                                chkFails[4] = true;
                                errorHandler(chkFails, false);
                            }
                        }
                        k++;
                        de++;
                    }

                    // 10
                    int index = 0;
                    sz = BLOCK_SIZE / sizeof(struct dirent);
                    while (index < sz)
                    {
                        if (dip[dirE->inum].type == 0 && dirE->inum != 0)
                        {
                            chkFails[10] = true;
                            errorHandler(chkFails, false);
                        }
                        index++;
                        de++;
                    }
                    dirBlk++;
                }
                // may have to chk indirect block here later
                // uint
                // still 10
                uint indirectIn[NINDIRECT];
                rsect(xint(dip[i].addrs[NDIRECT]), (char *)indirectIn);
                int indrIndex = 0;
                while (indrIndex < NINDIRECT)
                {
                    int index = 0;
                    struct dirent *dirE = (struct dirent *)(addr + (indirectIn[indrIndex]) * BLOCK_SIZE);
                    int sz = BLOCK_SIZE / sizeof(struct dirent);

                    while (index < sz)
                    {
                        chkFails[10] = true;
                        errorHandler(chkFails, false);
                        index++;
                        dirE++;
                    }
                    indrIndex++;
                }

                // check 12 in same "if"

                int directCounter = 0;
                int ind12 = 0;
                while (ind12 < sb->ninodes)
                {
                    if (dip[ind12].type == 1)
                    {
                        int indexer12 = 0;

                        while (indexer12 < NDIRECT)
                        {
                            struct dirent *dirE = (struct dirent *)(addr + (dip[ind12].addrs[indexer12]) * BLOCK_SIZE);
                            int in12 = 0;
                            int sz = BLOCK_SIZE / sizeof(struct dirent);

                            while (in12 < sz)
                            {
                                if (strcmp(dirE->name, "..") != 0 && strcmp(dirE->name, ".") != 0 && dirE->inum == i)
                                    directCounter++;

                                in12++;
                                dirE++;
                            }
                            indexer12++;
                        }

                        uint indirectIn[NINDIRECT];
                        rsect(xint(dip[ind12].addrs[NDIRECT]), (char *)indirectIn);

                        int indexIndirect = 0;

                        while (indexIndirect < NINDIRECT)
                        {
                            if (indirectIn[indexIndirect] <= 0)
                            {
                                indexIndirect++;
                                continue;
                            }
                            else
                            {
                                struct dirent *dirE = (struct dirent *)(addr + (indirectIn[indexIndirect]) * BLOCK_SIZE);
                                int temp = 0;
                                int sz = BLOCK_SIZE / sizeof(struct dirent);

                                while (temp < sz)
                                {
                                    if (strcmp(dirE->name, "..") != 0 && strcmp(dirE->name, ".") != 0 && dirE->inum == i)
                                        directCounter++;

                                    temp++;
                                    dirE++;
                                }
                            }
                            indexIndirect++;
                        }
                    }

                    ind12++;
                }

                if (ROOTINO != i && directCounter != 1)
                {
                    chkFails[12] = true;
                    errorHandler(chkFails, false);
                }
            } // end 10
        }

        // check 11, if type 2
        if (dip[i].type == 2)
        {
            int referenceCounter = 0;
            int index = 0;

            while (index < sb->inodes)
            {
                if (dip[index].type == 1)
                {
                    int inIndex = 0;

                    while (inIndex < NDIRECT)
                    {
                        struct dirent *dirE = (struct dirent *)(addr + (dip[index].addrs[inIndex]) * BLOCK_SIZE);
                        int dinIndex = 0;
                        int sz = BLOCK_SIZE / sizeof(struct dirent);

                        while (dinIndex < sz)
                        {
                            if (dirE->inum == i)
                                referenceCounter++;

                            dinIndex++;
                            dirE++;
                        }
                        inIndex++;
                    }
                }

                uint indirectIn[NINDIRECT];
                rsect(xint(dip[index].addrs[NDIRECT]), (char *)indirectIn);

                int indrIndex = 0;
                while (indrIndex < NINDIRECT)
                {
                    if (indirectIn[indrIndex] > 0)
                    {
                        struct dirent *dirE = (struct dirent *)(addr + (indirectIn[indrIndex]) * BLOCK_SIZE);
                        int sz = BLOCK_SIZE / sizeof(struct dirent);
                        int index2 = 0;

                        while (index2 < sz)
                        {
                            if (dirE->inum == i)
                                referenceCounter++;

                            index2++;
                            dirE++;
                        }
                    }
                    indrIndex++;
                }
                index++;
            }

            if (referenceCounter != dip[i].nlink)
            {
                chkFails[11] = true;
                errorHandler(chkFails, false);
            }

        } // end 11 i think

        // check dir blocks
        int dirBlk = 0;
        while (dirBlk < NDIRECT)
        {
            // allocated dir block
            if (dip[i].addrs[dirBlk] != 0)
            {
                // dir block has invalid addr
                if (dip[i].addrs[dirBlk] > sb->nblocks || dip[i].addrs[dirBlk] < 0)
                { // 2
                    chkFails[2] = true;
                    errorHandler(chkFails, true);
                }

                // check bitmap has 1 where block is
                char *bitMap = (char *)(addr + BBLOCK(0, sb->ninodes) * BLOCK_SIZE);
                int x = 1 << (dip[i].addrs[dirBlk] % 8);
                if ((bitMap[dip[i].addrs[dirBlk] / 8] & m) == 0)
                { // 5
                    chkFails[5] = true;
                    errorHandler(chkFails, false);
                }

                // verify that every dir block has bn
                int bnum = dip[i].addrs[dirBlk];
                int nxtI = i + 1;
                while (nxtI < sb->ninodes)
                {
                    if (dip[nxtI].type != 0)
                    {
                        // dir blks
                        int dirBlkNxt = 0;
                        while (dirBlkNxt < NDIRECT)
                        {
                            if (dip[nxtI].addrs[dirBlkNxt] == bnum)
                            { // 7
                                chkFails[7] = true;
                                errorHandler(chkFails, false);
                            }
                            dirBlkNxt++;
                        }
                    }
                    nxtI++;
                }
            }

            dirBlk++;
        }

        // indir block
        // checks: 2, 5, 8
        i = 0;
        if (dip[i].addrs[NDIRECT] != 0)
        { ////////////////////////////
            // indir block has invalid addr
            if (dip[i].addrs[NDIRECT] > sb->nblocks || dip[i].addrs[NDIRECT] < 0)
            { // 2
                chkFails[2] = true;
                errorHandler(chkFails, false);
            }

            char *bitMap = (char *)(addr + BBLOCK(0, sb->ninodes) * BLOCK_SIZE);
            int x = 1 << (dip[i].addrs[NDIRECT] % 8);
            if ((bitMap[dip[i].addrs[NDIRECT] / 8] & m) == 0)
            { // 5
                chkFails[5] = true;
                errorHandler(chkFails, false);
            }

            // still 5
            // check inside indirect block
            int indirBlk = 0;
            uint indirect[NINDIRECT];
            rsect(xint(dip[i].addrs[NDIRECT]), (char *)indirect);
            while (indirBlock < NINDIRECT)
            {
                if (indirect[indirBlock] != 0)
                {
                    char *bitMap = (char *)(addr + BBLOCK(0, sb->ninodes) * BLOCK_SIZE);
                    int x = 1 << (dip[i].addrs[NDIRECT] % 8);
                    if ((bitMap[indirect[indirBlk] / 8] & m) == 0)
                    { // 5
                        chkFails[5] = true;
                        errorHandler(chkFails, false);
                    }
                }
                indirBlock++;
            }

            // 8
            uint indirectOut[NINDIRECT];
            int indirBlkAddr = dip[i].addrs[NDIRECT];
            rsect(xint(indirBlkAddr), (char *)indirectOut);
            int nxtI = i + 1;
            while (nxtI < sb->ninodes)
            {
                if (dip[nxtI].type != 0)
                {
                    // indirblk used
                    if (dip[nxtI].addrs[NDIRECT] > 0)
                    {
                        // addr is already being used
                        if (dip[nxtI].addrs[NDIRECT] > 0)
                        {
                            chkFails[8] = true;
                            errorHandler(chkFails, false);
                        }

                        // store inner indir blk as well
                        uint indirectIn[NINDIRECT];
                        rsect(xint(dip[i].addrs[NDIRECT]), (char *)indrectIn);
                        int o = 0, in = 0;
                        while (o < NINDIRECT)
                        {
                            while (in < NINDIRECT)
                            {
                                if (indirectOut[o] == indirectIn[in] && indirect[o] != 0)
                                {
                                    chkFails[8] = true;
                                    errorHandler(chkFails, false);
                                }
                                in++;
                            }
                            o++;
                        }
                    }
                }
                nxtI++;
            }

            // // the addresses within indir block must also be valid
            // int indirBlk = 0;
            // // rsect
            // while (indirBlk < NINDIRECT){
            //   // 2
            // }
        }
        i++;
    }
    exit(0);
}

// all errors: go to when error occurred during traversal
void errorHandler(bool b[], bool isDir)
{
    // idx of arr represents errors 1 thru 12 inc.
    // if true, then print to stderr and exit
    if (b[1])
        fprintf(stderr, "ERROR: bad inode.\n");
    else if (b[2])
    {
        if (isDir)
            fprintf(stderr, "ERROR: bad direct address in inode.\n");
        else
            fprintf(stderr, "ERROR: bad indirect address in inode.\n");
    }
    else if (b[3])
        fprintf(stderr, "ERROR: root directory does not exist.\n");
    else if (b[4])
        fprintf(stderr, "ERROR: directory not properly formatted.\n");
    else if (b[5])
        fprintf(stderr, "ERROR: address used by inode but market free in bitmap.\n");
    else if (b[6])
        fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.\n");
    else if (b[7])
        fprintf(stderr, "ERROR: direct address used more than once.\n");
    else if (b[8])
        fprintf(stderr, "ERROR: indirect address used more than once.\n");
    else if (b[9])
        fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
    else if (b[10])
        fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
    else if (b[11])
        fprintf(stderr, "ERROR: bad reference count for file.\n");
    else if (b[12])
        fprintf(stderr, "ERROR: directory appears more than once in file system.\n");

    exit(1); // exit with error
}

void rsect(uint sec, void *buf)
{
    if (lseek(fsfd, sec * 512L, 0) != sec * 512L)
    {
        perror("lseek");
        exit(1);
    }
    if (lseek(fsfd, buf, 512) != 512)
    {
        perror("read");
        exit(1);
    }
}

uint xint(uint x)
{
    uint n;
    uchar *c = (uchar *)&n;
    c[0] = x;
    c[1] = x >> 8;
    c[2] = x >> 16;
    c[3] = x >> 24;

    return n;
}
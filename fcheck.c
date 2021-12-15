#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>   // for use of fstat
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>

//#include "types.h"
//#include "fs.h"

// prototypes
void errorHandler(bool[], bool);

#define BLOCK_SIZE (BSIZE)

int
main(int argc, char *argv[])
{
  int r,i,n,fsfd;
  char *addr;
  struct dinode *dip;
  struct superblock *sb;
  struct dirent *de;

  // other specifications: no image file is provided
  if(argc < 2){
    fprintf(stderr, "Usage: fcheck <file_system_image>");
    exit(1);
  }

  // open file image to be chked
  fsfd = open(argv[1], O_RDONLY);
  // image file did not open
  if(fsfd < 0){
    perror(argv[1]);
    exit(1);
  }


  ///////////////////////// ...Begin real changes... /////////////////////////////

  // Important info:

  // Block 0 = unused
  // Block 1 = super block
  // block 2 to ... = inodes
  // after = bitmap

  // searches start from root dir: ROOTINO 1
  // block size = 512
  // superblock has: size, number of data blocks (nblocks), and number of inodes (ninodes)
  // read superblock first to traverse for fschk
  
  // inode struct:
  // type = file or dir or device, in stat.h: T_DIR = 1, T_FILE = 2,T_DEV = 3

  // IPB = inodes per block

  // directory entry struct:
  // inum
  // names

  /* Dont hard code the size of file. Use fstat to get the size */
  // map file as part of memory (memory mapped I/O)
  struct stat sz;
  fstat(fsfd, &sz);   // fstat on file desc
  addr = mmap(NULL, sz.st_size, PROT_READ, MAP_PRIVATE, fsfd, 0);
  if (addr == MAP_FAILED){
    perror("mmap failed");
    exit(1);
  }

  /* read the super block */
  // ptr to superblock for direct access
  sb = (struct superblock *) (addr + 1 * BLOCK_SIZE);
  //printf("fs size %d, no. of blocks %d, no. of inodes %d \n", sb->size, sb->nblocks, sb->ninodes);

  /* read the inodes */
  // ptr to inode
  dip = (struct dinode *) (addr + IBLOCK((uint)0)*BLOCK_SIZE); 
  //printf("begin addr %p, begin inode %p , offset %d \n", addr, dip, (char *)dip -addr);

  // get the address of root dir 
  de = (struct dirent *) (addr + (dip[ROOTINO].addrs[0])*BLOCK_SIZE);

  ////////////////////////////// traverse fs ////////////////////////////////////////
  // the current inode
  int i = 0;
  // bool arr where each idx, 1 to 12, represents an error if true
  bool chkFails[13] = { 0 };

  // chk1: each inode must be unallocated or 1 of the 3 types (0 to 3 inc)
  bool isValidOrAllocated;

  // inode 1 must be a dir
  if (dip[ROOTNO].type != 1){   // 3
    chkFails[3] = true;
    errorHandler(chkFails, false);
  }

  // traverse all entries within the init direct block as indicated by de
  int j = 0;
  while (j < BSIZE/sizeof(struct dirent)){
    // verify parent dir -> ROOTINO
    if (strcmp(de->name, "..") == 0){
      if(de->inum == ROOTINO) {
        return;
      } else {
        chkFails[3] = true;
        errorHandler(chkFails, false);
      }
    }
    j++; de++;
  }

  // at this point the parent DNE
  chkFails[3] = true;
  errorHandler(chkFails, false);



  // iterate thru every inode
  while (i < sb->ninodes){
    isValidOrAllocated = dip[i].type >= 0 && dip[i].type <= 3
    if (!isValidOrAllocated){   // 1
      chkFails[1] = true;
      errorHandler(chkFails, false);
    }

    // allocated inode
    if (dip[i].type != 0){
      // inode is a dir: T_DIR
      if (dip[i].type == T_DIR){
        int dirBlk = 0;
        bool oneDot = false, twoDots = false;

        // direct block entries
        while (dirBlk < NDIRECT){
          struct dirent* de = (struct dirent *) (addr + (dip[i].addrs[dirBlk])*BLOCK_SIZE);
          int sz = dip[i].size/sizeof(struct dirent);
          int k = 0;
          while (k < sz){
            // located .. or . entry
            if (strcmp(de->name, ".") == 0) oneDot = true;
            if (strcmp(de->name, "..") == 0){
              twoDots = true;

              // verify . has correct definition
              if(i != de->inum){  // 4
                chkFails[4] = true;
                errorHandler(chkFails, false);
              }
            }
            k++; de++;
          }
          dirBlk++;
        }

        // may have to chk indirect block here later
        // uint
      }




      // check dir blocks
      int dirBlk = 0;
      while (dirBlk < NDIRECT){
        // allocated dir block
        if (dip[i].addrs[dirBlk] != 0){
          // dir block has invalid addr
          if (dip[i].addrs[dirBlk] > sb->nblocks || dip[i].addrs[dirBlk] < 0){   // 2
            chkFails[2] = true;
            errorHandler(chkFails, true);
          }

          // check bitmap has 1 where block is
          char* bitMap = (char*) (addr + BBLOCK(0, sb->ninodes)*BLOCK_SIZE);
          int x = 1 << (dip[i].addrs[dirBlk] % 8);
          if ((bitMap[dip[i].addrs[dirBlk] / 8] & m) == 0){   // 5
            chkFails[5] = true;
            errorHandler(chkFails, false);
          }
        }

        dirBlk++;
      }

      // indir block
      if (dip[i].addrs[NDIRECT] != 0){
        // indir block has invalid addr
        if (dip[i].addrs[NDIRECT] > sb->nblocks || dip[i].addrs[NDIRECT] < 0){    // 2
          chkFails[2] = true;
          errorHandler(chkFails, false);
        }

        // the addresses within indir block must also be valid
        int indirBlk = 0;
        // rsect
        while (indirBlk < NINDIRECT){
          // 2
        }
      }
    }
    else {

    }



    i++;
  }

  // // read root inode
  // // sz of inode, links to inode, and type of inode
  // printf("Root inode  size %d links %d type %d \n", dip[ROOTINO].size, dip[ROOTINO].nlink, dip[ROOTINO].type);

  // // print the entries in the first block of root dir 
  // n = dip[ROOTINO].size/sizeof(struct dirent);
  // for (i = 0; i < n; i++,de++){
 	//   printf(" inum %d, name %s ", de->inum, de->name);
  // 	printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);
  // }

  exit(0);
}

// all errors: go to when error occurred during traversal
void
errorHandler(bool b[], bool isDir)
{
  // idx of arr represents errors 1 thru 12 inc.
  // if true, then print to stderr and exit
  if (b[1]) fprintf(stderr, "ERROR: bad inode.");
  else if (b[2]) {
    if (isDir) fprintf(stderr, "ERROR: bad direct address in inode.");
    else fprintf(stderr, "ERROR: bad indirect address in inode.");
  }
  else if (b[3]) fprintf(stderr, "ERROR: root directory does not exist.");
  else if (b[4]) fprintf(stderr, "ERROR: directory not properly formatted.");
  else if (b[5]) fprintf(stderr, "ERROR: address used by inode but market free in bitmap.");
  else if (b[6]) fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.");
  else if (b[7]) fprintf(stderr, "ERROR: direct address used more than once.");
  else if (b[8]) fprintf(stderr, "ERROR: indirect address used more than once.");
  else if (b[9]) fprintf(stderr, "ERROR: inode marked use but not found in a directory.");
  else if (b[10]) fprintf(stderr, "ERROR: inode referred to in directory but marked free.");
  else if (b[11]) fprintf(stderr, "ERROR: bad reference count for file.");
  else if (b[12]) fprintf(stderr, "ERROR: directory appears more than once in file system.");

  exit(1);    // exit with error
}

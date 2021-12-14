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
void errorDetected(bool);

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

  ////////////////////////////// traverse fs ////////////////////////////////////////
  // the current inode
  int i = 0;
  // bool arr where each idx, 1 to 12, represents an error if true
  bool chkFails[13] = { 0 };


  // chk1: each inode must be unallocated or 1 of the 3 types (0 to 3 inc)
  bool isValidOrAllocated = dip[i].type >= 0 && dip[i].type <= 3;

  while (i < sb->ninodes){
    if (!isValidOrAllocated){   // 1
      fprintf(stderr, "ERROR: bad inode.");
      exit(1);
    }

    // allocated inode
    if (dip[i].type > 0) {
      
    }
    else {

    }

    i++;
  }

  // all error messages
  if (chk1Fail) {

  }

  // // read root inode
  // // sz of inode, links to inode, and type of inode
  // printf("Root inode  size %d links %d type %d \n", dip[ROOTINO].size, dip[ROOTINO].nlink, dip[ROOTINO].type);

  // // get the address of root dir 
  // de = (struct dirent *) (addr + (dip[ROOTINO].addrs[0])*BLOCK_SIZE);

  // // print the entries in the first block of root dir 
  // n = dip[ROOTINO].size/sizeof(struct dirent);
  // for (i = 0; i < n; i++,de++){
 	//   printf(" inum %d, name %s ", de->inum, de->name);
  // 	printf("inode  size %d links %d type %d \n", dip[de->inum].size, dip[de->inum].nlink, dip[de->inum].type);
  // }

  exit(0);
}

// all errors: go to when error occurred during traversal
void errorDetected(bool b) {
  if ()
}

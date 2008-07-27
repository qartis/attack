#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <libgen.h>
#include "attack.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef X_IRWXU
#define PERMS S_IRWXU|S_IRWXG|S_IRWXO
#else
#define PERMS 0666
#endif

char *uniqid(){
  char *str = malloc(16+1);
  str[0] = '.';
  int i = 1;
  for (;i<16;++i) {
    str[i] = 'a'+rand()%16;
  }
  str[16] = '\0';
  return str;
}

int duplicate(const char *from, const char *to){
  int f,t;
  char buf[4096];
  ssize_t len;

  f = open(from, O_RDONLY|O_BINARY);
  if (-1 == f){
    DEBUG("can't open %s: %s",from,strerror(errno));
    return 1;
  }

  t = open(to, O_WRONLY|O_CREAT|O_TRUNC|O_BINARY, PERMS);
  if (-1 == t){
    DEBUG("can't create %s: %s",to,strerror(errno));
    goto abort;
  }

  for(;;){
    len = read(f,buf,sizeof(buf));
    if (len < 0){
      DEBUG("failed to read file: %s",strerror(errno));
      goto abort;
    } else if (len > 0){
      if (write(t,buf,len) < 1){
        DEBUG("failed to write %ld bytes: %s",len,strerror(errno));
      }
    } else break;
  }

  DEBUG("done copying file");
  close(f);
  close(t);
  chmod(to,S_IRUSR|S_IWUSR|S_IXUSR);
  return 0;

abort:
  DEBUG("aborting patch");
  close(f);
  close(t);
  unlink(to);
  return 1;
}

static off_t offtin(u_char *buf){
	off_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

char *apply_patch(const char *patch_path, const char *my_path){
  int rc;
  DEBUG("patching %s with %s",my_path,patch_path);
  char *tempfile =uniqid();
  DEBUG("new patched file name: '%s'",tempfile);
  if (0 != duplicate(my_path,tempfile)){
    DEBUG("error duplicating file, aborting the whole shebang (should delete patch file)");
    free(tempfile);
    return NULL;
  }

  FILE *f, *cpf, *dpf, *epf;
  BZFILE * cpfbz2, * dpfbz2, * epfbz2;
	int cbz2err, dbz2err, ebz2err;
  ssize_t oldsize=0,newsize;
	ssize_t bzctrllen,bzdatalen;
	u_char header[32],buf[8];
	u_char *old=NULL, *new=NULL;
	off_t oldpos,newpos;
	off_t ctrl[3];
	off_t lenread;
  int i;

  /* Open patch file */
	if ((f = fopen(patch_path, "rb")) == NULL){
    DEBUG("Error opening %s",patch_path);
    goto bail;
  }

  if (fread(header, 1, 32, f) < 32) {
		if (feof(f)) DEBUG("corrupt patch?");
		DEBUG("error: fread");
    goto bail;
	}

  	/* Check for appropriate magic */
	if (memcmp(header, "BSDIFF40", 8) != 0){
    DEBUG("corrupt patch?");
    goto bail;
  }

	/* Read lengths from header */
	bzctrllen=offtin(header+8);
	bzdatalen=offtin(header+16);
	newsize=offtin(header+24);
	if((bzctrllen<0) || (bzdatalen<0) || (newsize<0)){
    DEBUG("corrupt patch?");
    goto bail;
  }

	/* Close patch file and re-open it via libbzip2 at the right places */
	if (fclose(f)){DEBUG("fclose");goto bail;}
	if ((cpf = fopen(patch_path, "rb")) == NULL){DEBUG("fopen");goto bail;}
	if (fseek(cpf, 32, SEEK_SET)){DEBUG("fseek");goto bail;}
	if ((cpfbz2 = BZ2_bzReadOpen(&cbz2err, cpf, 0, 0, NULL, 0)) == NULL){DEBUG("BZ2_bzReadOpen");goto bail;}
	if ((dpf = fopen(patch_path, "rb")) == NULL){DEBUG("fopen");goto bail;}
	if (fseek(dpf, 32 + bzctrllen, SEEK_SET)){DEBUG("fseeko");goto bail;}
	if ((dpfbz2 = BZ2_bzReadOpen(&dbz2err, dpf, 0, 0, NULL, 0)) == NULL){DEBUG("BZ2_bzReadOpen");goto bail;}
	if ((epf = fopen(patch_path, "rb")) == NULL){DEBUG("fopen");goto bail;}
	if (fseek(epf, 32 + bzctrllen + bzdatalen, SEEK_SET)){DEBUG("fseeko(%s, %lld)",patch_path,(long long)(32 + bzctrllen + bzdatalen));goto bail;}
	if ((epfbz2 = BZ2_bzReadOpen(&ebz2err, epf, 0, 0, NULL, 0)) == NULL){DEBUG("BZ2_bzReadOpen, bz2err = %d", ebz2err);goto bail;}

	if(((fd=open(tempfile,O_RDONLY|O_BINARY,0))<0) ||
		((oldsize=lseek(fd,0,SEEK_END))==-1) ||
		((old=malloc(oldsize+1))==NULL) ||
		(lseek(fd,0,SEEK_SET)!=0) ||
		(read(fd,old,oldsize)!=oldsize) ||
		(close(fd)==-1)){
    DEBUG("WHOOPS %s",tempfile);
    goto bail;
  }
	if((new=malloc(newsize+1))==NULL){
    DEBUG("malloc");
    goto bail;
  }

	oldpos=0;newpos=0;
	while(newpos<newsize) {
		/* Read control data */
		for(i=0;i<=2;i++) {
			lenread = BZ2_bzRead(&cbz2err, cpfbz2, buf, 8);
			if ((lenread < 8) || ((cbz2err != BZ_OK) && (cbz2err != BZ_STREAM_END))){
        DEBUG("corrupt patch");
        goto bail;
      }
			ctrl[i]=offtin(buf);
		};

		/* Sanity-check */
		if(newpos+ctrl[0]>newsize){
      DEBUG("corrupt patch");
      goto bail;
    }

		/* Read diff string */
		lenread = BZ2_bzRead(&dbz2err, dpfbz2, new + newpos, ctrl[0]);
		if ((lenread < ctrl[0]) ||
		    ((dbz2err != BZ_OK) && (dbz2err != BZ_STREAM_END))){
      DEBUG("corrupt patch: %ld (error code %d)",lenread,dbz2err);
      goto bail;
    }

		/* Add old data to diff string */
		for(i=0;i<ctrl[0];i++)
			if((oldpos+i>=0) && (oldpos+i<oldsize))
				new[newpos+i]+=old[oldpos+i];

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize){
      DEBUG("corrupt patch");
      goto bail;
    }

		/* Read extra string */
		lenread = BZ2_bzRead(&ebz2err, epfbz2, new + newpos, ctrl[1]);
    if ((lenread < ctrl[1]) ||
		    ((ebz2err != BZ_OK) && (ebz2err != BZ_STREAM_END))){
      DEBUG("corrupt patch");
      goto bail;
    }

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&cbz2err, cpfbz2);
	BZ2_bzReadClose(&dbz2err, dpfbz2);
	BZ2_bzReadClose(&ebz2err, epfbz2);
	if (fclose(cpf) || fclose(dpf) || fclose(epf)){
    DEBUG("fclose");
    goto bail;
  }
  
	/* Write the new file */
	if(((fd=open(tempfile,O_CREAT|O_TRUNC|O_WRONLY|O_BINARY,0666))<0) ||
		(write(fd,new,newsize)!=newsize) || (close(fd)==-1)){
    DEBUG("WHOOPS");
    goto bail;
  }

	free(new);
	free(old);

  DEBUG("binary successfully patched to %s",tempfile);
  return tempfile;

bail:
  DEBUG("patch sequence failed, cleaning up and bailing");
  if ((rc = remove(".patch"))){
    DEBUG("failed to remove .patch (%d): %s",rc,strerror(rc));
  }
  
  if ((rc = remove(tempfile))){
    DEBUG("failed to remove %s (%d): %s",tempfile,rc,strerror(rc));
  }

  if (new) free(new);
  if (old) free(old);
  if (tempfile) free(tempfile);

  return NULL;
}

void check_for_patch(int argc, char **argv){
  int retval;
  FILE *patch = fopen(".patch","rb");
  if (argc > 1 && 0 == strcmp(argv[1],"-one")){
    DEBUG("step one, renaming the original file to temp location");
    char *proper = argv[2];
    DEBUG("proper name was %s",proper);
    char *new = uniqid();
    DEBUG("renaming old version %s to temporary name %s",proper,new);
    if ((retval = rename(proper,new))){
      DEBUG("error renaming file: %s",strerror(errno));
      goto bailbailbail;
    }
    char *argv2[] = {NULL,NULL,NULL,NULL,NULL};
    argv2[0] = new;
    argv2[1] = "-two";
    argv2[2] = argv[0];
    argv2[3] = proper;
    execv(new,argv2);
    DEBUG("very big error: %s",strerror(errno));
    goto bailbailbail;
  } else if (argc > 1 && 0 == strcmp(argv[1],"-two")){
    DEBUG("step two, renaming the new binary to the original location");
    char *binary_name = argv[2];
    char *original_name = argv[3];
    DEBUG("I think the original name was %s, moving %s there",original_name,binary_name);
    if ((retval = rename(binary_name,original_name))){
      DEBUG("error moving original file to %s",binary_name);
      goto bailbailbail;
    }
    char *argv2[] = {NULL,NULL,NULL,NULL};
    argv2[0] = original_name;
    argv2[1] = "-three";
    argv2[2] = argv[0];
    execv(original_name,argv2);
    DEBUG("serious error: %s",strerror(errno));
    goto bailbailbail;
  } else if (argc > 1 && 0 == strcmp(argv[1],"-three")){
    DEBUG("step three! removing renamed old binary %s",argv[2]);
    if ((retval = remove(argv[2]))){
      DEBUG("error removing file: %s",strerror(errno));
      goto bailbailbail;
    }
    DEBUG("upgrade process done, new binary now loaded.. enjoy");
    return;
  } else if (patch != NULL){
    fclose(patch);
    DEBUG("patch exists, starting the rename sequence");
    char *tempfile = apply_patch(".patch",argv[0]);
    if (tempfile == NULL){
      DEBUG("patch procedure messed up, bailing");
      goto bailbailbail;
    } else {
      DEBUG("deleting patch, no more use");
      if ((retval = remove(".patch"))){
        DEBUG("error removing (%d): %s",errno,strerror(errno));
        goto bailbailbail;
      }
    }
    char *argv2[] = {NULL,NULL,NULL,NULL};
    argv2[0] = strdup(tempfile);
    argv2[1] = "-one";
    argv2[2] = argv[0];
    execv(tempfile,argv2);
    DEBUG("very big error: %s",strerror(errno));
    exit(1);
  }
  DEBUG("no patch, running normally");
  return;

bailbailbail:
  if (argc > 2 && strlen(argv[2]) == 16+1){
    DEBUG("removing stale file %s",argv[2]);
    remove(argv[2]);
  }
  if (argc > 3 && strlen(argv[3]) == 16+1){
    DEBUG("removing stale file %s",argv[3]);
    remove(argv[3]);
  }
  remove(".patch");
  DEBUG("cleaned up, removed all temp files and patch");
}

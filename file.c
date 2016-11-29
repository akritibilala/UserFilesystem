
#define FUSE_USE_VERSION 26
#include <stdio.h>
#include <fuse.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#define OSP4_DEBUG 0
long int memory;
long int sizeof_node;
typedef struct node
{
	struct node *child;
	struct node *parent;
	struct node *previous;
	struct node *next;
	struct stat *node_property;
	int check_directory;
	int node_level;
  char *node_information;
	char *complete_path;
  char *info;
}node; 
node *head=NULL;

void Display()
{
  node *temp=head;
  //fprintf(stdout, "%s\n", temp->node_information);
  temp=temp->child;
  while(temp!=NULL)
  {
    //fprintf(stdout, "%s\n", temp->node_information);
    temp=temp->next;
  }
}

node* insert_at(char* dir)
{
  int flag =0;
  node *temp;
  temp=head;
  //fprintf(stdout, "Node structure in insert_at\n");
  Display();
  while(flag ==0)
  {
    if(temp ==NULL)
    {//fprintf(stdout, "in null part\n");
      return NULL;
    }
    if(strcmp(temp->complete_path,dir)==0)
    {//fprintf(stdout, "match found\n");
      flag=1;
      return temp;
    }
    else if(strstr(dir,temp->complete_path)!=NULL)
    {//fprintf(stdout, "in strstr\n");
      temp=temp->child;
    }
    else
    {//fprintf(stdout, "in sibling\n");
      temp=temp->next;
    }
  }
  //fprintf(stdout, "exiting insert_at\n");
}

static int fuse_getattribute(const char *path, struct stat *stbuf)
{
  //fprintf(stdout,"in getattr\n");
  char *p=strdup(path);
	memset(stbuf,0,sizeof(struct stat));
  stbuf->st_uid =getuid();
  stbuf->st_gid =getgid();
  stbuf->st_atime=time(NULL);
  stbuf->st_mtime=time(NULL);
  if(strcmp(path,"/")==0)
  {
    stbuf->st_mode= S_IFDIR| 0755;
    stbuf->st_nlink =2;
  }
  else
  { 
    //fprintf(stdout,"checking path: %s\n",p);
    node *tmp;
    tmp= insert_at(p);
    //fprintf(stdout, "After insert_at in getattr\n");
    if (tmp==NULL)
    {
      printf("tmp is null\n");
      return -ENOENT;
    }
    if(tmp->check_directory==1)
    {
      //fprintf(stdout, "tmp is DIR\n");
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink =2;
      stbuf->st_size = tmp->node_property->st_size;

    }
    else
    {
      //fprintf(stdout, "tmp is file\n");
      stbuf ->st_mode = S_IFREG | 0666;
      stbuf ->st_nlink= 1;
      stbuf->st_size = tmp->node_property->st_size;
    }
  }
  //fprintf(stdout,"returning from getattr\n");
  free(p);
  return 0;
}
static int fuse_opendirectory(const char *path, struct fuse_file_info *fi)
{ 
	return 0;
}
static int fuse_chmod(const char *path, mode_t mode)
{
  return 0;
}
static int fuse_chown(const char *path, uid_t uid, gid_t gid)
{
  return 0;
}
static int fuse_rddirectory(const char *path, void *buf, fuse_fill_dir_t filler,off_t offset, struct fuse_file_info *fi)
{
	
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  node *read_from;
  char *p=strdup(path), *name;
  read_from = insert_at(p);
  //fprintf(stdout, "readdir after insert_at and node structure for parent: %s\n", read_from->node_information);
  Display();
  if(read_from !=NULL)
  {
    node *hell=read_from->child;
    while(hell != NULL)
    {
      name= strndup(hell->node_information, strlen(hell->node_information));
      //fprintf(stdout, "name: %s\n", name);
      filler(buf,name,NULL,0);
      free(name);
      hell=hell->next;
    }
    }
    else
    {
      return -ENOENT;
    }
    //fprintf(stdout, "Node structure in readdir\n");
    Display();
  //free(p);
  return 0;
}
static int fuse_mkdirectory(const char *path, mode_t mode)
{
  //fprintf(stderr,"in mkdir\n");
  char temp1[1024];
  char temp2[1024];
  char *file_name,*dir_name;
  //fprintf(stderr,"checking mem\n");
	if(memory- sizeof_node<0)
  {
    //fprintf(stderr,"memory fail\n");
    return -ENOSPC;
  }
 
  //fprintf(stderr,"doing strcpy\n");
  node *new_node = (node*)malloc(sizeof(node));
  strcpy(temp1,path);
  strcpy(temp2,path);
  //fprintf(stderr,"finished strcpy\n");
  file_name=basename(temp1);
  dir_name= dirname(temp2);
  //fprintf(stdout,"copying,file_name: %s\tdirname: %s\n",file_name,dir_name);
  //new_node->parent =NULL;
  new_node->node_information=strndup(file_name,strlen(file_name));
  new_node->complete_path=strndup(temp1,strlen(temp1));
  //fprintf(stdout,"done copy\n");
  new_node->next=NULL;
  new_node->previous=NULL;  // previous check once 
  new_node->child=NULL;
  new_node->check_directory=1;
  new_node->node_level=0;
  new_node->info=NULL;
  new_node->node_property = (struct stat*)malloc(sizeof(struct stat));
  new_node->node_property->st_uid=getuid();
  new_node->node_property->st_nlink =2;
  new_node->node_property->st_gid=getgid();
  new_node->node_property->st_mode = mode;

  node *parent_node= insert_at(dir_name);
  //fprintf(stderr,"parent_node name: %s and cp: %s\n",parent_node->node_information, parent_node->complete_path);
  if (parent_node==NULL)
  {//parent not exist
    exit(EXIT_FAILURE);
  }
  parent_node->node_property->st_nlink+=1;
  new_node->parent=parent_node;
  if(parent_node->child==NULL)
  {
    //fprintf(stderr,"first child\n");
    parent_node->child= new_node;
    new_node->previous=NULL;
  }
  else
  { node* traverse;
    //fprintf(stderr,"traversing nodes\n");
    traverse=parent_node->child;
    while(traverse->next !=NULL)
    {
      traverse=traverse->next;
    }
    traverse->next=new_node;
    new_node->previous=traverse;
  }
  Display();
  memory = memory- sizeof_node;

	return 0;
}
static int fuse_unlink(const char *path)
{ 
  if((path==NULL)||(strcmp(path,"/")==0))
  {
    return -EPERM;
  }
  node *p;
  char *copy=strdup(path);
  p= insert_at(copy);
	if(p->next == NULL && p->previous==NULL)
  {
    (p->parent)->child=NULL;
  }
  else if(p->next !=NULL && p->previous==NULL)
  {
    (p->parent)->child = (p->next);
    (p->next)->previous=NULL;
  }
  else if(p->next==NULL)
  {
    (p->previous)->next= NULL;
  }
  else
  {
    (p->previous)->next=(p->next);
    (p->next)->previous=(p->previous);
  }
  long int data;
  if(p->info !=NULL)
  {
    data= strlen(p->info);
  }
   memory = memory+ sizeof_node + data;
  free(p->node_information);
  free(p->complete_path);
  free(p->node_property);
  free(p);
  return 0;
}
static int fuse_rmdirectory(const char *path)
{  
  //fprintf(stdout, "in rmdir with path: %s\n", path);
  if((path==NULL)||(strcmp(path,"/")==0))
  {
    return -EPERM;
  }
  node *p;
  char *copy=strdup(path);
  p= insert_at(copy);
  //fprintf(stdout, "after insert_at with path: %s\n", p->node_information);
  if(p->child !=NULL)
  {
   return -ENOTEMPTY; 
  }
  //fprintf(stdout, "parent: %s\n", (p->parent)->node_information);
  if(p->next == NULL && p->previous==NULL)
  {//fprintf(stdout, "in null null\n");
    (p->parent)->child=NULL;
  }
  else if(p->next !=NULL && p->previous==NULL)
  {//fprintf(stdout, "in notnull null\n");
    (p->parent)->child = (p->next);
    (p->next)->previous=NULL;
  }
  else if(p->next==NULL)
  {//fprintf(stdout, "in next null\n");
    (p->previous)->next= NULL;
  }
  else
  {//fprintf(stdout, "in else\n");
    (p->previous)->next=(p->next);
    (p->next)->previous=(p->previous);
  }
   memory = memory+ sizeof_node;
  free(p->node_information);
  free(p->complete_path);
  free(p->node_property);
  free(p);
  Display();
	return 0;
}
static int fuse_open(const char *path, struct fuse_file_info *fi)
{  
  char *p =strdup(path);
  if(insert_at(p)==NULL)
  {
    return -ENOENT;
  }

	return 0;
}
static int fuse_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi)
{
  //fprintf(stdout, "in read with path: %s\n", path);
  node *p;
  
  size_t len;
  char *copy=strdup(path);
  p= insert_at(copy);
  if(p->check_directory == 1 )
  {
    return -EISDIR;
  }
  if(p->info !=NULL){

  len=strlen(p->info);
  if (offset < len)
  {
    if(offset+size >len)
    {
      size = len-offset;
    }
    memcpy(buf,(p->info) + offset,size);
    buf[size]='\0';
  }else{
    size=0;
  }
  }
  else{
    size=0;
  }
  return size;
}
static int fuse_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi)
{ 
  //fprintf(stdout, "in write with path: %s\n", path);
 if(memory<size)
 {
  return -ENOSPC;
 } 
 node *p;
  size_t len;
  char *copy=strdup(path);
  p= insert_at(copy);
  if(p->check_directory == 1 )
  {
    return -EISDIR;
  } 
  if(size>0)
  { if (p->info==NULL)
  {
    len=0;
  }else
  {
    len=strlen(p->info);
  }
    if(len==0)
    {//fprintf(stdout, "in len zero\n");
      offset=0;
      p->info= (char *)malloc(sizeof(char)*size);
      //fprintf(stdout, "after mem allocation\n");
      memcpy((p->info)+offset,buf,size);
      //fprintf(stdout, "after memcpy\n");
      p->node_property->st_size= (offset+size);
      p->node_property->st_ctime=time(NULL);
      p->node_property->st_mtime=time(NULL);
      //fprintf(stdout, "after node_property\n");
      memory=memory-size;

    }
    else{//fprintf(stdout, "in len not zero\n");
      if(offset>len)
      {
        offset=len;
      }
      char *loc;
      loc= (char *)realloc(p->info,sizeof(char)* (offset+size));
      if(loc ==NULL)
      {
        return -ENOSPC;
      }
      else{//fprintf(stdout, "in setting value\n");
        p->info=loc;
        memcpy((p->info)+offset,buf,size);
        p->node_property->st_size= (offset+size);
        p->node_property->st_ctime=time(NULL);
        p->node_property->st_mtime=time(NULL);
        memory=memory-size-offset;
      }
    }
    //fprintf(stdout, "after if in write\n");
  }

 return size;
}

int create(char *path)
{
  //fprintf(stderr,"create file \n");
  char temp1[1024];
  char temp2[1024];
  char *file_name,*dir_name;
  //fprintf(stderr,"checking mem\n");
  if(memory- sizeof_node<0)
  {
    //fprintf(stderr,"memory fail\n");
    return -ENOSPC;
  }
 
  //fprintf(stderr,"doing strcpy\n");
  node *new_node = (node*)malloc(sizeof(node));
  strcpy(temp1,path);
  strcpy(temp2,path);
  //fprintf(stderr,"finished strcpy\n");
  file_name=basename(temp1);
  dir_name= dirname(temp2);
  //fprintf(stdout,"copying,file_name: %s\tdirname: %s\n",file_name,dir_name);
  //new_node->parent =NULL;
  new_node->node_information=strndup(file_name,strlen(file_name));
  new_node->complete_path=strndup(temp1,strlen(temp1));
  //fprintf(stdout,"done copy\n");
  new_node->next=NULL;
  new_node->previous=NULL;  // previous check once 
  new_node->child=NULL;
  new_node->check_directory=0;
  new_node->node_level=0;
  new_node->info=NULL;
  new_node->node_property = (struct stat*)malloc(sizeof(struct stat));
  new_node->node_property->st_uid=getuid();
  new_node->node_property->st_nlink =1;
  new_node->node_property->st_gid=getgid();
  new_node->node_property->st_mode = S_IFREG | 0666;

  node *parent_node= insert_at(dir_name);
  //fprintf(stderr,"parent_node name: %s and cp: %s\n",parent_node->node_information, parent_node->complete_path);
  if (parent_node==NULL)
  {//parent not exist
    exit(EXIT_FAILURE);
  }
  //parent_node->node_property->st_nlink+=1;
  new_node->parent=parent_node;
  if(parent_node->child==NULL)
  {
    //fprintf(stderr,"first child\n");
    parent_node->child= new_node;
    new_node->previous=NULL;
  }
  else
  { node* traverse;
    //fprintf(stderr,"traversing nodes\n");
    traverse=parent_node->child;
    while(traverse->next !=NULL)
    {
      traverse=traverse->next;
    }
    traverse->next=new_node;
    new_node->previous=traverse;
  }
  Display();
  memory = memory- sizeof_node;
 return 0;
  
}
static int fuse_rename(const char *s,const char *d)
{ char *source= strdup(s);
  char *destination= strdup(d);
  node *get=insert_at(source);
  node *set=insert_at(destination);
  if(get==NULL)
  {
    return -ENOENT;
  }
  if(set==NULL)
  {
     if(get->check_directory ==1)
     {
       fuse_mkdirectory(destination , get->node_property->st_mode); 
       set=insert_at(destination);
         set->node_property->st_atime = get->node_property->st_atime;
        set->node_property->st_mtime = get->node_property->st_mtime;
        set->node_property->st_ctime = get->node_property->st_ctime;
        set->node_property->st_mode = get->node_property->st_mode;
        set->node_property->st_gid = get->node_property->st_gid;
        set->node_property->st_nlink = get->node_property->st_nlink;
        set->node_property->st_uid = get->node_property->st_uid;
        set->node_property->st_size= get->node_property->st_size;
       fuse_rmdirectory(source);
       return 0;  
     }
     else
     {
       create(destination);
       set=insert_at(destination);
        set->node_property->st_atime = get->node_property->st_atime;
        set->node_property->st_mtime = get->node_property->st_mtime;
        set->node_property->st_ctime = get->node_property->st_ctime;
        set->node_property->st_mode = get->node_property->st_mode;
        set->node_property->st_gid = get->node_property->st_gid;
        set->node_property->st_nlink = get->node_property->st_nlink;
        set->node_property->st_uid = get->node_property->st_uid;
        fuse_unlink(source);

       return 0;
     }
  }

  else
  {     if(get->check_directory ==0)
        {


        set->node_property->st_atime = get->node_property->st_atime;
        set->node_property->st_mtime = get->node_property->st_mtime;
        set->node_property->st_ctime = get->node_property->st_ctime;
        set->node_property->st_mode = get->node_property->st_mode;
        set->node_property->st_gid = get->node_property->st_gid;
        set->node_property->st_nlink = get->node_property->st_nlink;
        set->node_property->st_uid = get->node_property->st_uid;

        }
  }

  return 0;
}
static int fuse_utimens(const char * path, const struct timespec tv[2])
{
  return 0;
}
static int fuse_truncate(const char * path , off_t offset){
 
  return 0;
}
static int fuse_create(const char *path , mode_t mode, struct fuse_file_info *fi)
{
	
  //fprintf(stderr,"create file \n");
  char temp1[1024];
  char temp2[1024];
  char *file_name,*dir_name;
  //fprintf(stderr,"checking mem\n");
  if(memory- sizeof_node<0)
  {
    //fprintf(stderr,"memory fail\n");
    return -ENOSPC;
  }
 
  //fprintf(stderr,"doing strcpy\n");
  node *new_node = (node*)malloc(sizeof(node));
  strcpy(temp1,path);
  strcpy(temp2,path);
  //fprintf(stderr,"finished strcpy\n");
  file_name=basename(temp1);
  dir_name= dirname(temp2);
  //fprintf(stdout,"copying,file_name: %s\tdirname: %s\n",file_name,dir_name);
  //new_node->parent =NULL;
  new_node->node_information=strndup(file_name,strlen(file_name));
  new_node->complete_path=strndup(temp1,strlen(temp1));
  //fprintf(stdout,"done copy\n");
  new_node->next=NULL;
  new_node->previous=NULL;  // previous check once 
  new_node->child=NULL;
  new_node->check_directory=0;
  new_node->node_level=0;
  new_node->info=NULL;
  new_node->node_property = (struct stat*)malloc(sizeof(struct stat));
  new_node->node_property->st_uid=getuid();
  new_node->node_property->st_nlink =1;
  new_node->node_property->st_gid=getgid();
  new_node->node_property->st_mode = mode;

  node *parent_node= insert_at(dir_name);
  //fprintf(stderr,"parent_node name: %s and cp: %s\n",parent_node->node_information, parent_node->complete_path);
  if (parent_node==NULL)
  {//parent not exist
    exit(EXIT_FAILURE);
  }
  //parent_node->node_property->st_nlink+=1;
  new_node->parent=parent_node;
  if(parent_node->child==NULL)
  {
    //fprintf(stderr,"first child\n");
    parent_node->child= new_node;
    new_node->previous=NULL;
  }
  else
  { node* traverse;
    //fprintf(stderr,"traversing nodes\n");
    traverse=parent_node->child;
    while(traverse->next !=NULL)
    {
      traverse=traverse->next;
    }
    traverse->next=new_node;
    new_node->previous=traverse;
  }
  Display();
  memory = memory- sizeof_node;

  return 0;
  
}

  static struct fuse_operations fuse_commands =
 {
 	.getattr= fuse_getattribute,
 	.readdir= fuse_rddirectory,
 	.mkdir= fuse_mkdirectory,
 	.unlink= fuse_unlink,
 	.rmdir= fuse_rmdirectory,
 	.open= fuse_open,
 	.read= fuse_read,
 	.write= fuse_write,
 	.create= fuse_create,
 	.opendir= fuse_opendirectory,
  .utimens=fuse_utimens,
  .chmod= fuse_chmod,
  .chown= fuse_chown,
  .truncate=fuse_truncate,
  .rename=fuse_rename,
 };

  int fuse_initialization(char *argv[])
{
  
  node *initial_node = (node*)malloc(sizeof(node));
  initial_node->parent =NULL;
  initial_node->node_information= strndup(argv[1],strlen(argv[1]));
  //strcpy(initial_node->node_information,argv[1]);
  initial_node->next=NULL;
  initial_node->previous=NULL;
  initial_node->complete_path= strdup("/");
  //strcpy(initial_node->complete_path,initial_node->node_information);
  initial_node->child=NULL;
  initial_node->check_directory=1;
  initial_node->node_level=0;
  memory = memory- sizeof_node;
  initial_node->node_property = (struct stat*)malloc(sizeof(struct stat));
  initial_node->node_property->st_uid=0;
  initial_node->node_property->st_nlink =2;
  initial_node->node_property->st_gid=0;
  initial_node->node_property->st_mode = S_IFDIR | 0755;
  #if OSP4_DEBUG
  if(memory <0)
  { 
  	printf("memory out of bound\n");
  	return -ENOSPC;
  }
  #endif
  head=initial_node;
 	return 0;
 }

 int main(int argc, char *argv[])
 {
 	#if OSP4_DEBUG
 	if(argc !=3)
    {
    	printf("wrong arguments\n");
    	exit(EXIT_FAILURE);
    }
    #endif 
    //fprintf(stdout,"passed args\n");
    memory=  (long) atoi(argv[2]);
    memory= memory *1024*1024;
    sizeof_node= sizeof(node)+ sizeof(stat);
    fuse_initialization(argv);
    argc--;
    //fprintf(stdout,"%s\n",argv[1]);
    //fprintf(stdout,"%d\n",argc);
	return fuse_main(argc, argv, &fuse_commands, NULL);

 }



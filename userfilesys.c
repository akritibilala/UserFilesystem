#include <stdio.h>
#include <fuse.h>

long int memory;
typedef struct node
{
	struct node *child;
	struct node *parent;
	struct node *previous;
	struct node *next;
	struct stat *node_property;
	int check_directory;
	int node_level;
	char *node_name;
	char *node_information;
	char *complete_path;
}node; 
node *head;

 struct fuse_commands =
 {
 	.getattr= fuse_getattribute,
 	.readdir= fuse_rddirectory,
 	.mkdir= fuse_mkdirectory,
 	.unlink= fuse_unlink,
 	.rmdir= fuse_rmdirectory,
 	.close= fuse_close,
 	.open= fuse_open,
 	.read= fuse_read,
 	.write= fuse_write,
 	.create= fuse_create,
 	.opendir= fuse_opendirectory, 
 };

 int main(int argc, char *argv[])
 {   
 	
    if(argc !=3)
    {
    	exit(EXIT_FALIURE);
    }
    
    memory=  (long) atoi(argv[2]);
    memory= memory *1024*1024;
 }


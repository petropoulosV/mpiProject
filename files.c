#include <stdlib.h>
#include "files.h"


struct file *new_file(int id)
{
    struct file *tmp_file;

    tmp_file = (struct file *) malloc(sizeof(struct file) );

    tmp_file->id = id;
    tmp_file->version = 1;

    return tmp_file;
}
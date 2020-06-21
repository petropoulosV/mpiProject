#include <stdlib.h>
#include "files.h"


struct file *new_file(int id, int version)
{
    struct file *tmp_file;

    tmp_file = (struct file *) malloc(sizeof(struct file) );
    /* if(!tmp_file)
        ERRX(1, "Error malloc\n"); */

    tmp_file->id = id;
    tmp_file->version = version;

    return tmp_file;
}
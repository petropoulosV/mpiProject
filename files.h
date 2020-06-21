#ifndef _FILES_H_
#define _FILES_H_

struct registration
{
    int id;
    int counter;
    int type;
    int version;
};

struct file{
    int id;
    int version;
};

struct file *new_file(int id, int version);

#endif /* _FILES_H_ */
#ifndef DATA_FORMAT_H
#define DATA_FORMAT_H
typedef struct operand_s
{
	unsigned int a;
	unsigned int b;
} operand_t;

typedef struct result_s
{
	unsigned int c;
} result_t;

typedef struct student_{

    char name[32];
    unsigned int roll_no;
    char hobby[32];
    char dept[32];
} student_t; 
#endif

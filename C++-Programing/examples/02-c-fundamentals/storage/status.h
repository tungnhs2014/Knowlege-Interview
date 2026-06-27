#ifndef CH02_STATUS_H
#define CH02_STATUS_H

extern int system_status;

void set_system_status(int status);
int get_system_status(void);
unsigned int next_sequence(void);

#endif


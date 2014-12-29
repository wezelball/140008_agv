#ifndef GYROPROCEDURES_INCLUDED
#define GYROPROCEDURES_INCLUDED
#endif

int mymillis(void);
int timeval_subtract(struct timeval *result, struct timeval *t2, struct
timeval *t1);
void updateAngles();
void outputRobotPos();
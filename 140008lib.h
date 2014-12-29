#ifndef SERVER_140008LIB_H_INCLUDED
#define SERVER_140008LIB_H_INCLUDED
#endif
/* ^^ these are the include guards */

#define PRESENT 0	// mag sensor sees tape
#define ABSENT 1 	// mag sensor doesn't see tape

typedef enum { false = 0, true = !false} bool;
// I hate having to do this, but I can't figure out a better way for now

typedef struct robotPositionStruct {
  float gyroXangle;
  float AccXangle;
  float CFangleX;
  float gyroYangle;
  float AccYangle;
  float CFangleY;
} robotPosition;
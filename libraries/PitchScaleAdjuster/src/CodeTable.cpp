#include "CodeTable.h"

inline int size(int x[]){ return sizeof(x); }

/* 
 * CodeTypeClass XXXXX(key code, name string , ommits ,size of ommits); 
 */
int E_Major_ommits[] = { 1, 3, 6, 8, 10 };
CodeTypeClass E_Major(4,"Maj",E_Major_ommits,size(E_Major_ommits));

int C_Okinawa_ommits[] = { 1, 3, 5, 6, 8, 10 };
CodeTypeClass C_Okinawa(0,"Oki", C_Okinawa_ommits,size(C_Okinawa_ommits));

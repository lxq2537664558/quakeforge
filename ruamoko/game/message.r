#include "message.h"

void(float to, float f) WriteByte       = #52;
void(float to, float f) WriteChar       = #53;
void(float to, float f) WriteShort      = #54;
void(float to, float f) WriteLong       = #55;
void(float to, float f) WriteCoord      = #56;
void(float to, float f) WriteAngle      = #57;
void(float to, string s) WriteString    = #58;
void(float to, entity s) WriteEntity    = #59;
// sends the temp message to a set of clients, possibly in PVS or PHS
void(vector where, float set) multicast = #82;

/* Generated by the NeXT Project Builder 
   NOTE: Do NOT change this file -- Project Builder maintains it.
*/

#include <appkit/appkit.h>

void main(int argc, char *argv[]) {

    [Application new];
    if ([NSApp loadNibSection:"QuakeEd.nib" owner:NSApp withNames:NO])
	    [NSApp run];
	    
    [NSApp free];
    exit(0);
}

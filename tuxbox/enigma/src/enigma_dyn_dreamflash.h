#ifndef __enigma_dyn_dreamflash_h
#define __enigma_dyn_dreamflash_h

class eHTTPDynPathResolver;
void ezapFlashInitializeDyn(eHTTPDynPathResolver *dyn_resolver, bool lockWeb);
int installImage(eString, eString, eString);

#endif /* __enigma_dyn_flash_h */


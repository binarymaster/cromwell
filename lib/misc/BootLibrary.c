
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************

	2003-01-12 andy@warmcat.com  Created from other files, enhanced malloc
	                             greatly so it is more like a stdc one

*/

#include "boot.h"

//#ifndef XBE
void BootPciInterruptGlobalStackStateAndDisable(DWORD *dw) {	__asm__ __volatile__ (  "pushf; pop %%eax ; mov %%eax, (%%ebx); cli" : : "b" (dw)); }
void BootPciInterruptGlobalPopState(DWORD dw)  {	__asm__ __volatile__  (  "push %%ebx; popf" : : "b" (dw)); }
void BootPciInterruptEnable()  {	__asm__ __volatile__  (  "sti" ); }
//#else
//void BootPciInterruptGlobalStackStateAndDisable(DWORD *dw) {}
//void BootPciInterruptGlobalPopState(DWORD dw)  {}
//void BootPciInterruptEnable()  {}
//#endif

void * memcpy(void *dest, const void *src,  size_t size) {
//    bprintf("memcpy(0x%x,0x%x,0x%x);\n",dest,src,size);
#if 0
	BYTE * pb=(BYTE *)src, *pbd=(BYTE *)dest;
	while(size--) *pbd++=*pb++;

#else
		__asm__ __volatile__ (
              "    push %%esi    \n"
              "    push %%edi    \n"
              "    push %%ecx    \n"
             "    cld    \n"
              "    mov %0, %%esi \n"
              "    mov %1, %%edi \n"
              "    mov %2, %%ecx \n"
              "    push %%ecx    \n"
              "    shr $2, %%ecx \n"
              "    rep movsl     \n"
              "    pop %%ecx     \n"
              "    and $3, %%ecx \n"
              "    rep movsb     \n"
              : :"S" (src), "D" (dest), "c" (size)
		);

		__asm__ __volatile__ (
		          "    pop %ecx     \n"
              "    pop %edi     \n"
              "    pop %esi     \n"
		);
#endif
//	I2cSetFrontpanelLed(0x0f);
//    bprintf("memcpy done\n");
//		DumpAddressAndData(0xf0000, (BYTE *)0xf0000, 0x100);

	 return dest;
}

// int strlen(const char * sz) { int n=0; while(sz[n]) n++; return n; }
size_t strlen(const char * s)
{
int d0;
register int __res;
__asm__ __volatile__(
	"repne\n\t"
	"scasb\n\t"
	"notl %0\n\t"
	"decl %0"
	:"=c" (__res), "=&D" (d0) :"1" (s),"a" (0), "0" (0xffffffff));
return __res;
}


void * memset(void *dest, int data,  size_t size) {
//    bprintf("memset(0x%x,0x%x,0x%x);\n", dest, data, size);
    __asm__ __volatile__ (
              "    push %%eax    \n"
              "    push %%edi    \n"
              "    push %%ecx    \n"
              "    mov %0, %%edi \n"
              "    mov %1, %%eax \n"
              "    mov %2, %%ecx \n"
              "    shr $2, %%ecx \n"
              "    rep stosl     \n"
              "    pop %%ecx     \n"
              "    pop %%edi     \n"
              "    pop %%eax     \n"
              : : "S" (dest), "%eax" (data), "c" (size)
		);
	return dest;
}

int _memcmp(const BYTE *pb, const BYTE *pb1, int n) {
	while(n--) { if(*pb++ != *pb1++) return 1; }
	return 0;
}

	// this is the memory managemnt struct stored behind every allocation

typedef struct {
	void * m_pvNext;
	void * m_pvPrev;
	DWORD m_dwSentinel;
	int m_nLength; // negative means allocated length including mgt struct
} MEM_MGT;

MEM_MGT * pmemmgtStartAddressMemoryMangement;
#define SENTINEL_CONST 0xaa556b2
#define MERGE_IF_LESS_THAN_THIS_LEFT_OVER 0x100

void MemoryManagementInitialization(void * pvStartAddress, DWORD dwTotalMemoryAllocLength)
{
	pmemmgtStartAddressMemoryMangement=pvStartAddress;
	pmemmgtStartAddressMemoryMangement->m_dwSentinel=SENTINEL_CONST;
	pmemmgtStartAddressMemoryMangement->m_pvNext=NULL;
	pmemmgtStartAddressMemoryMangement->m_pvPrev=NULL;
	pmemmgtStartAddressMemoryMangement->m_nLength=dwTotalMemoryAllocLength;
}

void * malloc(size_t size) {
	MEM_MGT * pmemmgt=pmemmgtStartAddressMemoryMangement;
	DWORD dwInterruptState;
	BootPciInterruptGlobalStackStateAndDisable(&dwInterruptState);

	size+=sizeof(MEM_MGT);  // account for the fact that any block is prepended with management structure

		// have a look for first available block of equal or larger size

	while(pmemmgt) {
		ASSERT(pmemmgt->m_dwSentinel == SENTINEL_CONST);
		if(pmemmgt->m_nLength > size) {
			if((pmemmgt->m_nLength - size) <= (sizeof(MEM_MGT)+MERGE_IF_LESS_THAN_THIS_LEFT_OVER)) {
					// use whole of this block, there's not enough left to split it off
				pmemmgt->m_nLength=-pmemmgt->m_nLength;
				BootPciInterruptGlobalPopState(dwInterruptState);
				return (void *)(((BYTE *)pmemmgt)+sizeof(MEM_MGT));
			} else {
					// new guy takes up space at end of donor block
				MEM_MGT *pmemmgtNew=(MEM_MGT *)(((BYTE *)pmemmgt)+pmemmgt->m_nLength-size);
				pmemmgtNew->m_dwSentinel=SENTINEL_CONST;
				pmemmgtNew->m_pvNext=pmemmgt->m_pvNext;
				pmemmgtNew->m_pvPrev=pmemmgt;
				pmemmgtNew->m_nLength=-size;
				pmemmgt->m_nLength-=size;  // adjust donor length
				pmemmgt->m_pvNext=pmemmgtNew; // insert into linked list
				if(pmemmgtNew->m_pvNext!=NULL) {
					MEM_MGT * pmemmgtNext = (MEM_MGT *)pmemmgtNew->m_pvNext;
					ASSERT(pmemmgtNext->m_pvPrev == (void *)pmemmgt);
					pmemmgtNext->m_pvPrev=pmemmgtNew;
				}
				BootPciInterruptGlobalPopState(dwInterruptState);
				return (void *)(((BYTE *)pmemmgtNew)+sizeof(MEM_MGT));
			}
		}
		pmemmgt=(MEM_MGT *)pmemmgt->m_pvNext;
	}
	BootPciInterruptGlobalPopState(dwInterruptState);
	return NULL; // screwed, not enough memory
}

void free (void *ptr) {
	MEM_MGT * pmemmgt=(MEM_MGT *)(((BYTE *)ptr)-sizeof(MEM_MGT));
	MEM_MGT * pmemmgtPrev=(MEM_MGT *)pmemmgt->m_pvPrev;
	MEM_MGT * pmemmgtNext=(MEM_MGT *)pmemmgt->m_pvNext;
	DWORD dwInterruptState;
	BootPciInterruptGlobalStackStateAndDisable(&dwInterruptState);

	ASSERT(pmemmgt->m_dwSentinel == SENTINEL_CONST);

	pmemmgt->m_nLength=-pmemmgt->m_nLength; // change over to being free memory

		// try to combine with previous first
	if(pmemmgtPrev!=NULL) {
		if(pmemmgtPrev->m_nLength >0) { // free area previous too: previous guy takes us over
			pmemmgtPrev->m_nLength+=pmemmgt->m_nLength;
				// fix up linked list
			pmemmgtPrev->m_pvNext = pmemmgtNext;
			if(pmemmgtNext!=NULL) {
				pmemmgtNext->m_pvPrev =pmemmgtPrev;
			}
			BootPciInterruptGlobalPopState(dwInterruptState);
			return; // done
		}
	}
	if(pmemmgtNext!=NULL) {
		if(pmemmgtNext->m_nLength >0) { // free area next too: our guy takes over next
			pmemmgt->m_nLength+=pmemmgtNext->m_nLength;
				// fix up linked list
			pmemmgt->m_pvNext=pmemmgtNext->m_pvNext;
			if(pmemmgtNext->m_pvNext!=NULL) {
				MEM_MGT * pmemmgtNextNext=(MEM_MGT *)pmemmgtNext->m_pvNext;
				ASSERT(pmemmgtNextNext->m_pvPrev==pmemmgtNext);
				pmemmgtNextNext->m_pvPrev =pmemmgt;
			}
		}
	}
		// otherwise we have to leave it isolated, but marked as free now
	BootPciInterruptGlobalPopState(dwInterruptState);
}

int grub_strlen(const char *sz) {
	int n=0; while(*sz++) n++;
	return n;
}

char * strcpy(char *sz, const char *szc)
{
	char *szStart=sz;
	while(*szc) *sz++=*szc++;
	*sz='\0';
	return szStart;
}

char * _strncpy(char *sz, const char *szc, int nLenMax)
{
	char *szStart=sz;
	while((*szc) && (nLenMax--)) *sz++=*szc++;
	*sz='\0';
	return szStart;
}


int _strncmp(const char *sz1, const char *sz2, int nMax) {

	while((*sz1) && (*sz2) && nMax--) {
		if(*sz1 != *sz2) return (*sz1 - *sz2);
		sz1++; sz2++;
	}
	if(nMax==0) return 0;
	if((*sz1) || (*sz2)) return 0;
	return 0; // used up nMax
}


unsigned long GetMillisecondCount()
{
	DWORD dw1, dw2, * pdw;
	unsigned long ul[2];

	__asm__ __volatile__ (
		"rdtsc " : "=a" (dw1), "=d" (dw2)
	);
	pdw=(DWORD *)&ul;
	*pdw++=dw1;
	return ul[0];
}

void Sleep(int nMicroseconds)
{
	unsigned long ul=GetMillisecondCount();

	while(nMicroseconds--) {
		while((GetMillisecondCount()/733)==(ul/733)) ;
	}
}

void ListEntryInsertAfterCurrent(LIST_ENTRY *plistentryCurrent, LIST_ENTRY *plistentryNew)
{
	plistentryNew->m_plistentryPrevious=plistentryCurrent;
	plistentryNew->m_plistentryNext=plistentryCurrent->m_plistentryNext;
	plistentryCurrent->m_plistentryNext=plistentryNew;
	if(plistentryNew->m_plistentryNext!=NULL) {
		plistentryNew->m_plistentryNext->m_plistentryPrevious=plistentryNew;
	}
}

void ListEntryRemove(LIST_ENTRY *plistentryCurrent)
{
	if(plistentryCurrent->m_plistentryPrevious) {
		plistentryCurrent->m_plistentryPrevious->m_plistentryNext=plistentryCurrent->m_plistentryNext;
	}
	if(plistentryCurrent->m_plistentryNext) {
		plistentryCurrent->m_plistentryNext->m_plistentryPrevious=plistentryCurrent->m_plistentryPrevious;
	}
}

char *HelpGetToken(char *ptr,char token) {
	static char *old;
	char *mark;

	if(ptr != 0) old = ptr;
	mark = old;
	for(;*old != 0;old++) {
		if(*old == token) {
			*old = 0;
			old++;
			break;
		}
	}
	return mark;
}

void HelpGetParm(char *szBuffer, char *szOrig) {
	char *ptr,*copy;
	int nBeg = 0;
	int nCopy = 0;

	copy = szBuffer;
	for(ptr = szOrig;*ptr != 0;ptr++) {
		if(*ptr == ' ') nBeg = 1;
		if(*ptr != ' ' && nBeg == 1) nCopy = 1;
		if(nCopy == 1) {
			*copy = *ptr;
		 	copy++;
		}
	}
	*copy = 0;
}

char *strrchr0(char *string, char ch) {
        char *ptr = string;
	while(*ptr != 0) {
		if(*ptr == ch) {
			return ptr;
		} else {
			ptr++;
		}
	}
	return NULL;
}

#include "system_cpuid.h"

__inline int __get_cpuid_max (unsigned int __leaf, unsigned int *__sig)
{
    unsigned int __eax, __ebx, __ecx, __edx;
#if __i386__
    int __cpuid_supported;

    __asm("  pushfl\n"
          "  popl   %%eax\n"
          "  movl   %%eax,%%ecx\n"
          "  xorl   $0x00200000,%%eax\n"
          "  pushl  %%eax\n"
          "  popfl\n"
          "  pushfl\n"
          "  popl   %%eax\n"
          "  movl   $0,%0\n"
          "  cmpl   %%eax,%%ecx\n"
          "  je     1f\n"
          "  movl   $1,%0\n"
          "1:"
        : "=r" (__cpuid_supported) : : "eax", "ecx");
    if (!__cpuid_supported)
	return 0;
#endif
#if !defined (ANDROID)
    __cpuid(__leaf, __eax, __ebx, __ecx, __edx);
#endif
    if (__sig)
	*__sig = __ebx;
    return __eax;
}

__inline int __get_cpuid (unsigned int __leaf, unsigned int *__eax,
                                 unsigned int *__ebx, unsigned int *__ecx,
                                 unsigned int *__edx)
{
    unsigned int __max_leaf = __get_cpuid_max(__leaf & 0x80000000, 0);

    if (__max_leaf == 0 || __max_leaf < __leaf)
	return 0;

#if !defined (ANDROID)
    __cpuid(__leaf, *__eax, *__ebx, *__ecx, *__edx);
#endif
    return 1;
}

__inline int __get_cpuid_count (unsigned int __leaf,
                                       unsigned int __subleaf,
                                       unsigned int *__eax, unsigned int *__ebx,
                                       unsigned int *__ecx, unsigned int *__edx)
{
    unsigned int __max_leaf = __get_cpuid_max(__leaf & 0x80000000, 0);

    if (__max_leaf == 0 || __max_leaf < __leaf)
	return 0;

#if !defined (ANDROID)
    __cpuid_count(__leaf, __subleaf, *__eax, *__ebx, *__ecx, *__edx);
#endif
    return 1;
}

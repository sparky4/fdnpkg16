/*
 *  This file contains code for displaying the Intel Cpu identification
 *  that has been performed by CheckCpuType() function.
 *
 *  COPYRIGHT (c) 1998 valette@crf.canon.fr
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.OARcorp.com/rtems/license.html.
 *
 *  $Id: displayCpu.c,v 1.4 1998/08/24 16:59:20 joel Exp $
 *
 *  Heavily modified for Watt-32. Added support for Watcom-386,
 *  calculate CPU speed.  G. Vanem  <giva@bgnett.no> 2000
 */

/*
 * Tell us the machine setup..
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "wattcp.h"
#include "timer.h"
#include "misc.h"

#define CPU_TEST
#include "cpumodel.h"

#if !defined(HAVE_UINT64)
#error I need 64-bit integer support
#endif

static char Cx86_step = 0;

static char *Cx86_type[] = {
     "unknown", "1.3", "1.4", "1.5", "1.6", "2.4",
     "2.5", "2.6", "2.7 or 3.7", "4.2"
   };

const char *i486model (unsigned int nr)
{
  static char *model[] = {
              "0", "DX", "SX", "DX/2", "4", "SX/2", "6",
              "DX/2-WB", "DX/4", "DX/4-WB", "10", "11", "12", "13",
              "Am5x86-WT", "Am5x86-WB"
            };
  if (nr < DIM(model))
     return (model[nr]);
  return (NULL);
}

const char *i586model (unsigned int nr)
{
  static char *model[] = {
              "0", "Pentium 60/66", "Pentium 75+", "OverDrive PODP5V83",
              "Pentium MMX", NULL, NULL, "Mobile Pentium 75+",
              "Mobile Pentium MMX"
            };
  if (nr < DIM(model))
     return (model[nr]);
  return (NULL);
}

const char *Cx86model (void)
{
  BYTE   nr6x86 = 0;
  static const char *model[] = {
                    "unknown", "6x86", "6x86L", "6x86MX", "MII"
                  };

  switch (x86_type)
  {
    case 5:
         /* cx8 flag only on 6x86L */
         nr6x86 = ((x86_capability & X86_CAPA_CX8) ? 2 : 1);
	 break;
    case 6:
	 nr6x86 = 3;
	 break;
    default:
	 nr6x86 = 0;
  }

  /* We must get the stepping number by reading DIR1
   */
  outp (0x22, 0xff);
  x86_mask = inp (0x23);

  switch (x86_mask)
  {
    case 0x03:
	 Cx86_step = 1;		/* 6x86MX Rev 1.3 */
	 break;
    case 0x04:
	 Cx86_step = 2;		/* 6x86MX Rev 1.4 */
	 break;
    case 0x05:
	 Cx86_step = 3;		/* 6x86MX Rev 1.5 */
	 break;
    case 0x06:
	 Cx86_step = 4;		/* 6x86MX Rev 1.6 */
	 break;
    case 0x14:
	 Cx86_step = 5;		/* 6x86 Rev 2.4 */
	 break;
    case 0x15:
	 Cx86_step = 6;		/* 6x86 Rev 2.5 */
	 break;
    case 0x16:
	 Cx86_step = 7;		/* 6x86 Rev 2.6 */
	 break;
    case 0x17:
	 Cx86_step = 8;		/* 6x86 Rev 2.7 or 3.7 */
	 break;
    case 0x22:
	 Cx86_step = 9;		/* 6x86L Rev 4.2 */
	 break;
    default:
	 Cx86_step = 0;
  }
  return (model[nr6x86]);
}

const char *i686model (unsigned int nr)
{
  static const char *model[] = {
                    "PPro A-step", "Pentium Pro"
                  };
  if (nr < DIM(model))
     return (model[nr]);
  return (NULL);
}

struct cpu_model_info {
       int   x86;
       char *model_names[16];
     };

static const struct cpu_model_info amd_models[] = {
  { 4,
    { NULL, NULL, NULL, "DX/2", NULL, NULL, NULL, "DX/2-WB", "DX/4",
      "DX/4-WB", NULL, NULL, NULL, NULL, "Am5x86-WT", "Am5x86-WB"
    }
  },
  { 5,
    { "K5/SSA5 (PR-75, PR-90, PR-100)", "K5 (PR-120, PR-133)",
      "K5 (PR-166)", "K5 (PR-200)", NULL, NULL,
      "K6 (166 - 266)", "K6 (166 - 300)", "K6-2 (200 - 450)",
      "K6-3D-Plus (200 - 450)", NULL, NULL, NULL, NULL, NULL, NULL
    }
  },
};

const char *AMDmodel (void)
{
  char *p = NULL;
  int   i;

  if (x86_model < 16)
  {
    for (i = 0; i < DIM(amd_models); i++)
        if (amd_models[i].x86 == x86_type)
        {
          p = amd_models[i].model_names[(int)x86_model];
          break;
        }
  }
  return (p);
}

const char *getmodel (int x86, int model)
{
  const  char *p = NULL;
  static char nbuf[12];

  if (!strncmp (x86_vendor_id, "Cyrix", 5))
     p = Cx86model();
  else if (!strcmp (x86_vendor_id, "AuthenticAMD"))
     p = AMDmodel();
#if 0
  else if (!strcmp (x86_vendor_id, "UMC UMC UMC "))
     p = UMCmodel();
  else if (!strcmp (x86_vendor_id, "NexGenDriven"))
     p = NexGenModel();
  else if (!strcmp (x86_vendor_id, "CentaurHauls"))
     p = CentaurModel();
  else if (!strcmp (x86_vendor_id, "RiseRiseRise"))  /* Rise Technology */
     p = RiseModel();
  else if (!strcmp (x86_vendor_id, "GenuineTMx86"))  /* Transmeta */
     p = TransmetaModel();
  else if (!strcmp (x86_vendor_id, "Geode by NSC"))  /* National Semiconductor */
     p = NationalModel();
#endif
  else   /* Intel */
  {
    switch (x86)
    {
      case 4:
	   p = i486model (model);
	   break;
      case 5:
           p = i586model (model);   /* Pentium I */
	   break;
      case 6:
           p = i686model (model);   /* Pentium II */
	   break;
      case 7:
           p = "Pentium 3";
           break;
      case 8:
           p = "Pentium 4";
           break;
    }
  }
  if (p)
     return (p);

  sprintf (nbuf, "%d", model);
  return (nbuf);
}

void print_cpu_info (void)
{
  static const char *x86_cap_flags[] = {
               "FPU", "VME", "DE", "PSE", "TSC", "MSR", "PAE", "MCE",
               "CX8", "APIC", "FSC", "SEP", "MTRR", "PGE", "MCA", "CMOV",
               "PAT", "PSE36", "PSN", "CFLSH", "20??", "DTES", "ACPI", "MMX",
               "FXSR", "SSE", "SSE2", "SSNOOP", "28??", "ACC", "IA64", "31??"
             };
  int i;

  printf ("cpu      : %d86\n", x86_type);
  printf ("model    : %s\n", x86_have_cpuid ? getmodel(x86_type,x86_model) :
                                              "unknown");

  if (x86_vendor_id[0] == '\0')
     strcpy (x86_vendor_id, "unknown");

  printf ("vendor_id: %s\n", x86_vendor_id);

  if (x86_mask)
  {
    if (!strncmp (x86_vendor_id, "Cyrix", 5))
         printf ("stepping : %s\n", Cx86_type[(int)Cx86_step]);
    else printf ("stepping : %d\n", x86_mask);
  }
  else
    printf ("stepping : unknown\n");

  printf ("fpu      : %s\n", (x86_hard_math  ? "yes" : "no"));
  printf ("cpuid    : %s\n", (x86_have_cpuid ? "yes" : "no"));
  printf ("flags    :");

  for (i = 0; i < 32; i++)
  {
    if (x86_capability & (1 << i))
       printf (" %s", x86_cap_flags[i]);
  }
  printf ("\n");
}

static void print_cpu_serial_number (void)
{
  printf ("Serial # : ");
  if ((x86_capability & X86_CAPA_PSN) && x86_have_cpuid)
  {
    DWORD eax, ebx, ecx, edx;

    get_cpuid (3, &eax, &ebx, &ecx, &edx);
    printf ("%04X-%04X-%04X-%04X-%04X-%04X\n",
            (WORD)(ebx >> 16), (WORD)(ebx & 0xFFFF),
            (WORD)(ecx >> 16), (WORD)(ecx & 0xFFFF),
            (WORD)(edx >> 16), (WORD)(edx & 0xFFFF));
  }
  else
    printf ("not present\n");
}

void print_reg (DWORD reg, const char *what)
{
  BYTE a = (BYTE)(reg & 255);
  BYTE b = (BYTE)(reg >>  8) & 255;
  BYTE c = (BYTE)(reg >> 16) & 255;
  BYTE d = (BYTE)(reg >> 24);

  printf ("%s: %08lX, %c%c%c%c\n", what, reg,
          isprint(a) ? a : '.',
          isprint(b) ? b : '.',
          isprint(c) ? c : '.',
          isprint(d) ? d : '.');
}

void print_cpuid_info (void)
{
  DWORD eax, ebx, ecx, edx;

  printf ("\ncpuid,0 (name)\n");
  get_cpuid (0, &eax, &ebx, &ecx, &edx);
  print_reg (eax, "EAX");
  print_reg (ebx, "EBX");
  print_reg (edx, "EDX");
  print_reg (ecx, "ECX");

  printf ("\ncpuid,1 (family)\n");
  get_cpuid (1, &eax, &ebx, &ecx, &edx);
  print_reg (eax, "EAX");
  print_reg (ebx, "EBX");
  print_reg (ecx, "ECX");
  print_reg (edx, "EDX");

  printf ("\ncpuid, 80000002h (AMD/P4)\n");
  get_cpuid (0x80000002, &eax, &ebx, &ecx, &edx);
  print_reg (eax, "EAX");
  print_reg (ebx, "EBX");
  print_reg (ecx, "ECX");
  print_reg (edx, "EDX");

  printf ("\ncpuid, 80000003h (AMD/P4)\n");
  get_cpuid (0x80000003, &eax, &ebx, &ecx, &edx);
  print_reg (eax, "EAX");
  print_reg (ebx, "EBX");
  print_reg (ecx, "ECX");
  print_reg (edx, "EDX");

  printf ("\ncpuid, 80000004h (AMD/P4)\n");
  get_cpuid (0x80000004, &eax, &ebx, &ecx, &edx);
  print_reg (eax, "EAX");
  print_reg (ebx, "EBX");
  print_reg (ecx, "ECX");
  print_reg (edx, "EDX");
}

int main (void)
{
  uint64 Hz;

  init_misc();
  print_cpu_info();
  print_cpu_serial_number();

  Hz = get_cpu_speed();
  if (Hz)
       printf ("clock    : %.3f MHz\n", (double)Hz/1E6);
  else printf ("clock    : RDTSC not supported\n");

  if (x86_have_cpuid)
       print_cpuid_info();
  else puts ("No CPUID");
  return (0);
}


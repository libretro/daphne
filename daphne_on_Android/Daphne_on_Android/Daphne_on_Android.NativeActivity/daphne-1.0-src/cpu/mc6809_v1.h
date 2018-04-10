/*
 *  Module d'émulation des micro-circuits Motorola MC68xx:
 *    - microprocesseur MC6809E
 *    - PIA MC6846
 *    - PIA MC6821
 *
 *  Copyright (C) 1996 Sylvain Huet, 1999 Eric Botcazou.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 *  Module     : mc6809.h
 *  Version    : 2.62
 *  Créé par   : Sylvain Huet 1996
 *  Modifié par: Eric Botcazou 30/11/2000
 *
 *  Emulateur du microprocesseur Motorola MC6809E
 *
 *  version 1.0: émulation fonctionnelle
 *  version 2.0: horloge interne, interface
 *  version 2.1: interruption du timer
 *  version 2.2: nouvelles valeurs de retour des fonctions d'éxécution
 *  version 2.3: encapsulation complète du module
 *  version 2.4: ajout d'un masque d'écriture des registres
 *  version 2.5: ajout d'une fonction trace (mode DEBUG)
 *  version 2.6: nouvelles commandes externes (RESET, NMI, FIRQ)
 *               correction mineure du mode indexé 5-bit
 *               Fetch devient FetchInstr et utilise des unsigned char
 *               suppression d'un inline inutile
 *  version 2.7: nouvelle interface de manipulation de l'état du MC6809E
 */
#if 0

#ifndef MC6809_H
#define MC6809_H

#ifdef DEBUG
#include <stdio.h>

extern FILE *mc6809_ftrace;
#endif

#include "types.h"

#include "6809infc.h"

#define UINT32 uint32
typedef UINT32 mc6809_clock_t;

#define MC6809_FETCH_BUFFER_SIZE  5
#define MC6809_TIMER_DISABLED     (mc6809_clock_t) -1

#define MC6809_REGS_CC_FLAG        (1<<0)
#define MC6809_REGS_DP_FLAG        (1<<1)
#define MC6809_REGS_AR_FLAG        (1<<2)
#define MC6809_REGS_BR_FLAG        (1<<3)
#define MC6809_REGS_XR_FLAG        (1<<4)
#define MC6809_REGS_YR_FLAG        (1<<5)
#define MC6809_REGS_UR_FLAG        (1<<6)
#define MC6809_REGS_SR_FLAG        (1<<7) 
#define MC6809_REGS_PC_FLAG        (1<<8)   
#define MC6809_REGS_CPUCLOCK_FLAG  (1<<9)
#define MC6809_REGS_CPUTIMER_FLAG  (1<<10)
#define MC6809_REGS_MAX_FLAG       11

struct MC6809_REGS
{
	uint8 cc;
	uint16 dp;
	uint16 ar;
	uint16 br;
	uint16 xr;
	uint16 yr;
	uint16 ur;
	uint16 sr;
	uint16 pc;
    mc6809_clock_t cpu_clock;
    mc6809_clock_t cpu_timer;
};


struct MC6809_INTERFACE
{
    void (*FetchInstr)(uint16, uint8 []);
    uint8 (*LoadByte)(uint16);
    uint16 (*LoadWord)(uint16);
    void (*StoreByte)(uint16, uint8);
    void (*StoreWord)(uint16, uint16);
    int  (*TrapCallback)(struct MC6809_REGS *);
};

extern void mc6809_Init(const struct MC6809_INTERFACE *);
extern void mc6809_GetRegs(struct MC6809_REGS *);
extern void mc6809_SetRegs(const struct MC6809_REGS *, int);
extern void mc6809_SetTimer(mc6809_clock_t, void (*)(void *), void *);
extern void mc6809_Reset(void);

//extern int  mc6809_StepExec(unsigned int);
extern unsigned int mc6809_StepExec(unsigned int);	// MATT changed this to match callback specs

// added by MPO
extern unsigned int mc6809_GetPC();

// added by MPO
extern const char *mc6809_info(void *unused, int regnum);

extern int  mc6809_TimeExec(mc6809_clock_t);
extern mc6809_clock_t mc6809_clock(void);
extern int  mc6809_irq;
extern int  mc6809_nmi;
extern int  mc6809_firq;

#endif
#endif

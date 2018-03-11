/******************************************************************************
 *
 *		MOVEM.C
 *		Routines for the MOVEM instruction and the REG directive
 *
 *    Function: movem()
 *		Builds MOVEM instructions. The size of the instruction
 *		is given by the size argument (assumed to be word if
 *		not specified). The label argument points to the label
 *		appearing on the source line containing the MOVEM
 *		instruction, if any, and the op argument points to the
 *		operands of the MOVEM instruction. The routine returns
 *		an error code in *errorPtr by the standard mechanism.
 *
 *		reg()
 *		Defines a special register list symbol to be used as an
 *		argument for the MOVEM instruction. The size argument
 *		reflects the size code appended to the REG directive,
 *		which should be empty. The label argument points to the
 *		label appearing on the source line containing the REG
 *		directive (which must be specified), and the op
 *		argument points to a register list which is the new
 *		value of the symbol. The routine returns an error code
 *		in *errorPtr by the standard mechanism.
 *
 *	 Usage:	movem(size, label, op, errorPtr)
 *		int size;
 *		char *label, *op;
 *		int *errorPtr;
 *
 *		reg(size, label, op, errorPtr)
 *		int size;
 *		char *label, *op;
 *		int *errorPtr;
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/9/86
 *
 *   Copyright 1990-1991 North Carolina State University. All Rights Reserved.
 *
 *****************************************************************************/


#include <stdio.h>
#include <ctype.h>
#include "asm.h"


/* Define bit masks for the legal addressing modes of MOVEM */

#define ControlAlt  (AnInd | AnIndDisp | AnIndIndex | AbsShort | AbsLong)
#define DestModes   (ControlAlt | AnIndPre)
#define SourceModes (ControlAlt | AnIndPost | PCDisp | PCIndex)

extern int loc;
extern char pass2;

void
movem(int size, char *label, char *op, int *errorPtr)
{
	char *p;
	int status;
	unsigned short regList, temp, instMask;
	int i;
	opDescriptor memOp;

	/* Pick mask according to size code (only .W and .L are valid) */
	if (size == WORD)
		instMask = 0x4880;
	else if (size == LONG)
		instMask = 0x48C0;
	else {
		if (size)
			NEWERROR(*errorPtr, INV_SIZE_CODE);
		instMask = 0x4880;
	}
	/* Define the label attached to this instruction */
	if (*label)
		define(label, loc, pass2, errorPtr);

	/* See if the instruction is of the form MOVEM <reg_list>,<ea> */
	status = OK;
	/* Parse the register list */
	p = evalList(op, &regList, &status);
	if (status == OK && *p == ',') {
		/* Parse the memory address */
		p = opParse(++p, &memOp, &status);
		NEWERROR(*errorPtr, status);
		if (status < ERROR) {
			/* Check legality of addressing mode */
			if (memOp.mode & DestModes) {
				/* It's good, now generate the instruction */
				if (pass2) {
					output(instMask | effAddr(&memOp),
					       WORD);
					loc += 2;
					/* If the addressing mode is address
					   register indirect with predecrement,
					   reverse the bits in the register
					   list mask */
					if (memOp.mode == AnIndPre) {
						temp = regList;
						regList = 0;
						for (i = 0; i < 16; i++) {
							regList <<= 1;
							regList |=
							    (temp & 1);
							temp >>= 1;
						}
					}
					output(regList, WORD);
					loc += 2;
				} else
					loc += 4;
				extWords(&memOp, size, errorPtr);
				return;
			} else {
				NEWERROR(*errorPtr, INV_ADDR_MODE);
				return;
			}
		}
	}

	/* See if the instruction is of the form MOVEM <ea>,<reg_list> */
	status = OK;
	/* Parse the effective address */
	p = opParse(op, &memOp, &status);
	NEWERROR(*errorPtr, status);
	if (status < ERROR && *p == ',') {
		/* Check the legality of the addressing mode */
		if (memOp.mode & SourceModes) {
			/* Parse the register list */
			status = OK;
			p = evalList(++p, &regList, &status);
			if (status == OK) {
				/* Everything's OK, now build the instruction */
				if (pass2) {
					output(instMask | 0x0400 |
					       effAddr(&memOp), WORD);
					loc += 2;
					output(regList, WORD);
					loc += 2;
				} else
					loc += 4;
				extWords(&memOp, 4, errorPtr);
				return;
			}
		} else {
			NEWERROR(*errorPtr, INV_ADDR_MODE);
			return;
		}
	}

	/* If the instruction isn't of either form, then return an error */
	NEWERROR(*errorPtr, status);
}


void
reg(int size, char *label, char *op, int *errorPtr)
{
	symbolDef *symbol;
	unsigned short regList;

	if (size)
		NEWERROR(*errorPtr, INV_SIZE_CODE);
	if (!*op) {
		NEWERROR(*errorPtr, SYNTAX);
		return;
	}
	op = evalList(op, &regList, errorPtr);
	if (*errorPtr < SEVERE) {
		if (!*label) {
			NEWERROR(*errorPtr, LABEL_REQUIRED);
		} else {
			int status = OK;
			symbol = define(label, regList, pass2, &status);
			NEWERROR(*errorPtr, status);
			if (status < ERROR)
				symbol->flags |= REG_LIST_SYM;
		}
	}
}


/* Define a couple of useful tests */

#define isTerm(c)   (c == ',' || c == '/' || c == '-' || isspace(c) || !c)
#define isRegNum(c) ((c >= '0') && (c <= '7'))

char *
evalList(char *p, unsigned short *listPtr, int *errorPtr)
{
	char reg1, reg2, r;
	unsigned short regList;
	char symName[SIGCHARS + 1];
	int i;
	symbolDef *symbol;
	int status;

	regList = 0;
	/* Check whether the register list is specified
	   explicitly or as a register list symbol */
	if ((p[0] == 'A' || p[0] == 'D') && isRegNum(p[1]) && isTerm(p[2])) {
		/* Assume it's explicit */
		while (TRUE) {	/* Loop will be exited via return */
			if ((p[0] == 'A' || p[0] == 'D') && isRegNum(p[1])) {
				if (p[0] == 'A')
					reg1 = 8 + p[1] - '0';
				else
					reg1 = p[1] - '0';
				if (p[2] == '/') {
					/* Set the bit the for a single register */
					regList |= (1 << reg1);
					p += 3;
				} else if (p[2] == '-')
					if ((p[3] == 'A' || p[3] == 'D')
					    && isRegNum(p[4])
					    && isTerm(p[5])) {
						if (p[5] == '-') {
							NEWERROR(*errorPtr,
								 SYNTAX);
							return NULL;
						}
						if (p[3] == 'A')
							reg2 =
							    8 + p[4] - '0';
						else
							reg2 = p[4] - '0';
						/* Set all the bits corresponding to registers
						   in the specified range */
						if (reg1 < reg2)
							for (r = reg1;
							     r <= reg2;
							     r++)
								regList |=
								    (1 <<
								     r);
						else
							for (r = reg2;
							     r <= reg1;
							     r++)
								regList |=
								    (1 <<
								     r);
						if (p[5] != '/') {
							/* End of register list found - return its value */
							*listPtr = regList;
							return p + 5;
						}
						p += 6;
					} else {
						/* Invalid character found - return the error */
						NEWERROR(*errorPtr,
							 SYNTAX);
						return NULL;
				} else {
					/* Set the bit the for a single register */
					regList |= (1 << reg1);
					/* End of register list found - return its value */
					*listPtr = regList;
					return p + 2;
				}
			} else {
				/* Invalid character found - return the error */
				NEWERROR(*errorPtr, SYNTAX);
				return NULL;
			}
		}
	} else {
		/* Try looking in the symbol table for a register list symbol */
		if (!isalpha(*p) && *p != '.') {
			NEWERROR(*errorPtr, SYNTAX);
			return NULL;
		}
		i = 0;
		/* Collect characters of the symbol's name
		   (only SIGCHARS characters are significant) */
		do {
			if (i < SIGCHARS)
				symName[i++] = *p;
			p++;
		} while (isalnum(*p) || *p == '.' || *p == '_'
			 || *p == '$');
		/* Check for invalid syntax */
		if (!isspace(*p) && *p != ',' && *p) {
			NEWERROR(*errorPtr, SYNTAX);
			return NULL;
		}
		symName[i] = '\0';
		/* Look up the name in the symbol table, resulting
		   in a pointer to the symbol table entry */
		status = OK;
		symbol = lookup(symName, FALSE, &status);
		if (status < SEVERE)
			/* The register list symbol must be
			   previously defined in the program */
			if (status == UNDEFINED) {
				NEWERROR(*errorPtr, status);
			} else if (pass2 && !(symbol->flags & BACKREF)) {
				NEWERROR(*errorPtr, REG_LIST_UNDEF);
			} else {
				if (symbol->flags & REG_LIST_SYM)
					*listPtr = symbol->value;
				else {
					NEWERROR(*errorPtr, NOT_REG_LIST);
					*listPtr = 0x1234;
				}
		} else {
			NEWERROR(*errorPtr, status);
			*listPtr = 0;
		}
		return p;
	}
}

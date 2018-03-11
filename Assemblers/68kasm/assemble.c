/******************************************************************************
 *
 *		ASSEMBLE.C
 *		Assembly Routines for 68000 Assembler
 *
 *    Function: processFile()
 *		Assembles the input file. For each pass, the function
 *		passes each line of the input file to assemble() to be
 *		assembled. The routine also makes sure that errors are
 *		printed on the screen and listed in the listing file
 *		and keeps track of the error counts and the line
 *		number.
 *
 *		assemble()
 *		Assembles one line of assembly code. The line argument
 *		points to the line to be assembled, and the errorPtr
 *		argument is used to return an error code via the
 *		standard mechanism. The routine first determines if the
 *		line contains a label and saves the label for later
 *		use. It then calls instLookup() to look up the
 *		instruction (or directive) in the instruction table. If
 *		this search is successful and the parseFlag for that
 *		instruction is TRUE, it defines the label and parses
 *		the source and destination operands of the instruction
 *		(if appropriate) and searches the flavor list for the
 *		instruction, calling the proper routine if a match is
 *		found. If parseFlag is FALSE, it passes pointers to the
 *		label and operands to the specified routine for
 *		processing.
 *
 *	 Usage: processFile()
 *
 *		assemble(line, errorPtr)
 *		char *line;
 *		int *errorPtr;
 *
 *      Author: Paul McKee
 *		ECE492    North Carolina State University
 *
 *        Date:	12/13/86
 *
 *   Copyright 1990-1991 North Carolina State University. All Rights Reserved.
 *
 *****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include "asm.h"
//#pragma comment(lib, "msvcrt.lib")


extern int loc;			/* The assembler's location counter */
extern char pass2;		/* Flag set during second pass */
extern char endFlag;		/* Flag set when the END directive is encountered */
extern char continuation;	/* TRUE if the listing line is a continuation */
extern int lineNum;
extern int errorCount, warningCount;

extern char line[256];		/* Source line */
extern FILE *inFile;		/* Input file */
extern FILE *listFile;		/* Listing file */
extern char listFlag;

void assemble(char *line, int *errorPtr);

void
processFile(void)
{
	char capLine[256];
	int error, pass;

	pass2 = FALSE;
	for (pass = 0; pass < 2; pass++) {
		loc = 0;
		lineNum = 1;
		endFlag = FALSE;
		errorCount = warningCount = 0;
		while (!endFlag && fgets(line, 256, inFile)) {
			strcap(capLine, line);
			error = OK;
			continuation = FALSE;
			if (pass2 && listFlag)
				listLoc();
			assemble(capLine, &error);
			if (pass2) {
				if (error > MINOR)
					errorCount++;
				else if (error > WARNING)
					warningCount++;
				if (listFlag) {
					listLine();
					printError(listFile, error, -1);
				}
				printError(stderr, error, lineNum);
			}
			lineNum++;
		}
		if (!pass2) {
			pass2 = TRUE;
		}
		rewind(inFile);
	}
}

void
assemble(char *line, int *errorPtr)
{
	instruction *tablePtr;
	flavor *flavorPtr;
	opDescriptor source, dest;
	char *p, *start, label[SIGCHARS + 1], size, f;
	int sourceParsed, destParsed;
	unsigned short mask, i;

	p = start = skipSpace(line);
	if (*p && *p != '*') {
		i = 0;
		do {
			if (i < SIGCHARS)
				label[i++] = *p;
			p++;
		} while (isalnum(*p) || *p == '.' || *p == '_' || *p == '$');
		label[i] = '\0';
		if ((isspace(*p) && start == line) || *p == ':') {
			if (*p == ':')
				p++;
			p = skipSpace(p);
			if (*p == '*' || !*p) {
				define(label, loc, pass2, errorPtr);
				return;
			}
		} else {
			p = start;
			label[0] = '\0';
		}
		p = instLookup(p, &tablePtr, &size, errorPtr);
		if (*errorPtr > SEVERE)
			return;
		p = skipSpace(p);
		if (!tablePtr->parseFlag) {
			(*tablePtr->exec) (size, label, p, errorPtr);
			return;
		}
		// Move location counter to a word boundary and fix
		// the listing before assembling an instruction
		if (loc & 1) {
			loc++;
			listLoc();
		}
		if (*label)
			define(label, loc, pass2, errorPtr);
		if (*errorPtr > SEVERE)
			return;
		sourceParsed = destParsed = FALSE;
		flavorPtr = tablePtr->flavorPtr;
		for (f = 0; f < tablePtr->flavorCount; f++, flavorPtr++) {
			if (!sourceParsed && flavorPtr->source) {
				p = opParse(p, &source, errorPtr);
				if (*errorPtr > SEVERE)
					return;
				sourceParsed = TRUE;
			}
			if (!destParsed && flavorPtr->dest) {
				if (*p != ',') {
					NEWERROR(*errorPtr, SYNTAX);
					return;
				}
				p = opParse(p + 1, &dest, errorPtr);
				if (*errorPtr > SEVERE)
					return;
				if (!isspace(*p) && *p) {
					NEWERROR(*errorPtr, SYNTAX);
					return;
				}
				destParsed = TRUE;
			}
			if (!flavorPtr->source) {
				mask = pickMask(size, flavorPtr, errorPtr);
				(*flavorPtr->exec)(mask, errorPtr);
				return;
			} else if ((source.mode & flavorPtr->source) &&
				   !flavorPtr->dest) {
				if (!isspace(*p) && *p) {
					NEWERROR(*errorPtr, SYNTAX);
					return;
				}
				mask = pickMask(size, flavorPtr, errorPtr);
				(*flavorPtr->exec)(mask, size,
						   &source, &dest,
						   errorPtr);
				return;
			} else if (source.mode & flavorPtr->source
				   && dest.mode & flavorPtr->dest) {
				mask = pickMask(size, flavorPtr, errorPtr);
				(*flavorPtr->exec)(mask, size,
						   &source, &dest,
						   errorPtr);
				return;
			}
		}
		NEWERROR(*errorPtr, INV_ADDR_MODE);
	}
}


int
pickMask(int size, flavor * flavorPtr, int *errorPtr)
{
	if (!size || size & flavorPtr->sizes) {
		if (size & (BYTE | SHORT))
			return flavorPtr->bytemask;
		if (!size || size == WORD)
			return flavorPtr->wordmask;
		return flavorPtr->longmask;
	}
	NEWERROR(*errorPtr, INV_SIZE_CODE);

	return flavorPtr->wordmask;
}

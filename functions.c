
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "functions.h"

#define LINESIZE 11     // Max size of an input line


// these are the structures used in this simulator


// global variables
// register file
int regfile[32];
// instruction memory
int instmem[100];  // only support 100 static instructions
// data memory
int datamem[1000];
// program counter
int pc;

/* load
 *
 * Given the filename, which is a text file that 
 * contains the instructions stored as integers 
 *
 * You will need to load it into your global structure that 
 * stores all of the instructions.
 *
 * The return value is the maxpc - the number of instructions
 * in the file
 */
int load(char *filename)
{
    FILE *fin = fopen(filename, "r");
    char *line = NULL;
    int c, maxpc;

    for (maxpc = 0, c = getc(fin); c != EOF; maxpc++, c = getc(fin)) {
        char line[LINESIZE + 1];
        int i;
        for (i = 0; i < LINESIZE && c != '\n'; i++, c = getc(fin)) {
            line[i] = c;
        }
        line[i] = '\0';

        instmem[maxpc] = atoi(line);
    }
    fclose(fin);
	return maxpc - 1;
}

/* fetch
 *
 * This fetches the next instruction and updates the program counter.
 * "fetching" means filling in the inst field of the instruction.
 */
void fetch(InstInfo *instruction)
{
    instruction->inst = instmem[pc];        // Fill in the inst field
    pc++;                                   // Update the program counter
}

/* decode
 *
 * This decodes an instruction.  It looks at the inst field of the 
 * instruction.  Then it decodes the fields into the fields data 
 * member.  The first one is given to you.
 *
 * Then it checks the op code.  Depending on what the opcode is, it
 * fills in all of the signals for that instruction.
 */
void decode(InstInfo *instruction)
{
	// fill in the signals and fields
	int val = instruction->inst;
	int op, func;
	instruction->fields.op = (val >> 26) & 0x03f;
	// fill in the rest of the fields here
	instruction->fields.func = val & 0x03f;		
	if (instruction->fields.op == 48) {
		instruction->fields.rd = (val >> 11) &  0x01f;
		instruction->fields.imm = 0;
	} else if (instruction->fields.func == 36 || instruction->fields.func == 34) { 
		instruction->fields.imm = val & 0x03ffffff;
	} else {
		instruction->fields.imm = val & 0x0ffff;
	}
	if (instruction->fields.func != 36 && instruction->fields.func != 34) {
		instruction->fields.rs = (val >> 21) & 0x01f;
		instruction->fields.rt = (val >> 16) & 0x01f;
	} 
		
	
	// now fill in the signals
	//add
	if (instruction->fields.func == 10)
	{
		instruction->signals.aluop = 001;
		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 0;
		instruction->signals.btype = 00;
		instruction->signals.rdst = 1;
		instruction->signals.rw = 1;
		sprintf(instruction->string,"add $%d, $%d, $%d",
			instruction->fields.rd, instruction->fields.rs, 
			instruction->fields.rt);
		instruction->destreg = instruction->fields.rd;
	}
	//subi
	if (instruction->fields.op == 28)
	{
		instruction->signals.aluop = 101;
		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 1;
		instruction->signals.btype = 00;
		instruction->signals.rdst = 0;
		instruction->signals.rw = 1;
		sprintf(instruction->string,"subi $%d, $%d, $%d",
			instruction->fields.rt, instruction->fields.rs, 
			instruction->fields.imm);
		instruction->destreg = instruction->fields.rt;
	}
	//or
	if (instruction->fields.func == 48)
	{
		instruction->signals.aluop = 100;
		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 0;
		instruction->signals.btype = 00;
		instruction->signals.rdst = 1;
		instruction->signals.rw = 1;
		sprintf(instruction->string,"or $%d, $%d, $%d",
			instruction->fields.rd, instruction->fields.rs, 
			instruction->fields.rt);
		instruction->destreg = instruction->fields.rd;
	}
	//xor
	if (instruction->fields.func == 20)
	{
		instruction->signals.aluop = 011;
		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 0;
		instruction->signals.btype = 00;
		instruction->signals.rdst = 1;
		instruction->signals.rw = 1;
		sprintf(instruction->string,"xor $%d, $%d, $%d",
			instruction->fields.rd, instruction->fields.rs, 
			instruction->fields.rt);
		instruction->destreg = instruction->fields.rd;
	}
	//slt
	if (instruction->fields.func == 15 && instruction->fields.op == 48)
	{
		instruction->signals.aluop = 110;
		instruction->signals.mw = 0;
		instruction->signals.mr = 0;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 0;
		instruction->signals.btype = 00;
		instruction->signals.rdst = 1;
		instruction->signals.rw = 1;
		sprintf(instruction->string,"slt $%d, $%d, $%d",
			instruction->fields.rd, instruction->fields.rs, 
			instruction->fields.rt);
		instruction->destreg = instruction->fields.rd;
	}
	//lw
	if (instruction->fields.op == 6)
	{
		instruction->signals.aluop = 001;
		instruction->signals.mw = 0;
		instruction->signals.mr = 1;
		instruction->signals.mtr = 1;
		instruction->signals.asrc = 1;
		instruction->signals.btype = 00;
		instruction->signals.rdst =-1;
		instruction->signals.rw = 0;
		sprintf(instruction->string,"lw $%d, %d($%d)",
			instruction->fields.rt, instruction->fields.imm, 
			instruction->fields.rs);
		instruction->destreg = instruction->fields.rt;
	}
	//sw
	if (instruction->fields.op == 2)
	{
		instruction->signals.aluop = 001;
		instruction->signals.mw = 1;
		instruction->signals.mr = -1;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 1;
		instruction->signals.btype = 00;
		instruction->signals.rdst =-1;
		instruction->signals.rw = 0;
		sprintf(instruction->string,"sw $%d, %d($%d)",
			instruction->fields.rt, instruction->fields.imm, 
			instruction->fields.rs);
		instruction->destreg = instruction->fields.rt;
	}
	//bge	
	if (instruction->fields.op == 39)
	{
		instruction->signals.aluop = 101;
		instruction->signals.mw = 0;
		instruction->signals.mr = -1;
		instruction->signals.mtr = 0;
		instruction->signals.asrc = 0;
		instruction->signals.btype = 10;
		instruction->signals.rdst =-1;
		instruction->signals.rw = 0;
		sprintf(instruction->string,"bge $%d, %d, $%d",
			instruction->fields.rs, instruction->fields.rt, 
			instruction->fields.imm);
		instruction->destreg = instruction->fields.rt;
	}

	// fill in s1data and input2
}

/* execute
 *
 * This fills in the aluout value into the instruction and destdata
 */

void execute(InstInfo *instruction)
{

}

/* memory
 *
 * If this is a load or a store, perform the memory operation
 */
void memory(InstInfo *instruction)
{

}

/* writeback
 *
 * If a register file is supposed to be written, write to it now
 */
void writeback(InstInfo *instruction)
{
}


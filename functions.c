#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "functions.h"

#define LINESIZE 11     // Max size of an input line

#define is_add     instruction->fields.func == 10
#define is_subi    instruction->fields.op == 28 
#define is_or      instruction->fields.func == 48
#define is_xor     instruction->fields.func == 20
#define is_slt     instruction->fields.func == 15 && instruction->fields.op == 48
#define is_lw      instruction->fields.op == 6 
#define is_sw      instruction->fields.op == 2  
#define is_bge     instruction->fields.op == 39
#define is_j       instruction->fields.op == 36
#define is_jal     instruction->fields.op == 34

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

// Return Address
int ra;

typedef enum { R_format = 1, I_format = 2, J_format = 3 } InstFormat;
// 000 = and, 100 = or, 001 = add, 101 = sub, 010 = not, 011 = xor, 110 = slt
typedef enum {  INV = -1 ,
                AND = 000,
                ADD = 001,
                NOT = 010,
                XOR = 011,
                OR  = 100,
                SUB = 101,
                SLT = 110          
             } ALUOps;

typedef enum {  BGE = 1, JAL = 2, J = 3 } Jumps;

// Returns the format of the given instruction
// Note that the unused registers in the instruction
// MUST be set to -1 for the function to work!
InstFormat getFormat (InstInfo *instruction) {
    if (instruction->fields.rs != -1) {
        if (instruction->fields.rt != -1) {
            if (instruction->fields.rd != -1)
                return R_format;
            else
                return I_format;
        } else {
            // The setting up of the registers has failed, either the opcode is very wrong or the simulator is incorrect
            fprintf(stderr, "ERROR: Invalid Instruction %d\n", instruction->inst);
            exit(-1);
        }
    } else {
        return J_format;
    }
}

void setPCWithInfo(Jumps jump, int aluout, int jsize) {
    switch (jump) {
        case BGE:
            if (aluout >= 0)  // Branch
                pc += jsize;
            break;
        case JAL:
            ra = pc;
            pc = jsize;
            break;
        case J:
            pc = jsize;
            break;
    }
}

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
    pc++; //update program counter
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
	//int op, func;

	instruction->fields.op      = (val >> 26) & 0x03f;
	instruction->fields.func    = val & 0x03f;		

	if (instruction->fields.op == 48) {
        // Intruction with no imm
		instruction->fields.rd  = (val >> 11) &  0x01f;
		instruction->fields.imm = 0;
	} else if (instruction->fields.func == 36 || instruction->fields.func == 34) { 
        // Intruction with no rd
		instruction->fields.imm = val & 0x03ffffff;
		instruction->fields.rd  = -1;
	} else {
        // Intruction with no rd 
		instruction->fields.imm = val & 0x0ffff;
		instruction->fields.rd  = -1;
	}

	if (instruction->fields.func != 36 && instruction->fields.func != 34) {
		instruction->fields.rs  = (val >> 21) & 0x01f;
		instruction->fields.rt  = (val >> 16) & 0x01f;
	} else {
        // Instruction is of the J-Format
		instruction->fields.rs  = -1;
		instruction->fields.rt  = -1;
    }

	// now fill in the signals
	if (is_add) {           // add
		instruction->signals.aluop  = ADD;
		instruction->signals.mw     = 0;
		instruction->signals.mr     = 0;
		instruction->signals.mtr    = 0;
		instruction->signals.asrc   = 0;
		instruction->signals.btype  = 00;
		instruction->signals.rdst   = 1;
		instruction->signals.rw     = 1;
		sprintf(instruction->string,"add $%d, $%d, $%d",
			instruction->fields.rd, instruction->fields.rs, 
			instruction->fields.rt);
	} else if (is_subi)	{   // subi
		instruction->signals.aluop  = SUB;
		instruction->signals.mw     = 0;
		instruction->signals.mr     = 0;
		instruction->signals.mtr    = 0;
		instruction->signals.asrc   = 1;
		instruction->signals.btype  = 00;
		instruction->signals.rdst   = 0;
		instruction->signals.rw     = 1;
		sprintf(instruction->string,"subi $%d, $%d,  %d",
			instruction->fields.rt, instruction->fields.rs, 
			instruction->fields.imm);
	} else if (is_or) {     // or
		instruction->signals.aluop  = OR;
		instruction->signals.mw     = 0;
		instruction->signals.mr     = 0;
		instruction->signals.mtr    = 0;
		instruction->signals.asrc   = 0;
		instruction->signals.btype  = 00;
		instruction->signals.rdst   = 1;
		instruction->signals.rw     = 1;
		sprintf(instruction->string,"or $%d, $%d, $%d",
			instruction->fields.rd, instruction->fields.rs, 
			instruction->fields.rt);
	} else if (is_xor) {    // xor
		instruction->signals.aluop  = XOR;
		instruction->signals.mw     = 0;
		instruction->signals.mr     = 0;
		instruction->signals.mtr    = 0;
		instruction->signals.asrc   = 0;
		instruction->signals.btype  = 00;
		instruction->signals.rdst   = 1;
		instruction->signals.rw     = 1;
		sprintf(instruction->string,"xor $%d, $%d, $%d",
                instruction->fields.rd, instruction->fields.rs, 
                instruction->fields.rt);
    } else if (is_slt) {    // slt
        instruction->signals.aluop  = SLT;
        instruction->signals.mw     = 0;
        instruction->signals.mr     = 0;
        instruction->signals.mtr    = 0;
        instruction->signals.asrc   = 0;
        instruction->signals.btype  = 00;
        instruction->signals.rdst   = 1;
        instruction->signals.rw     = 1;
        sprintf(instruction->string,"slt $%d, $%d, $%d",
                instruction->fields.rd, instruction->fields.rs, 
                instruction->fields.rt);
    } else if (is_lw) {     // lw
        instruction->signals.aluop  = ADD;
        instruction->signals.mw     = 0;
        instruction->signals.mr     = 1;
        instruction->signals.mtr    = 1;
        instruction->signals.asrc   = 1;
        instruction->signals.btype  = 00;
        instruction->signals.rdst   = -1;
        instruction->signals.rw     = 0;
        sprintf(instruction->string,"lw $%d, %d ($%d)",
                instruction->fields.rt, instruction->fields.imm, 
                instruction->fields.rs);
    } else if (is_sw) { 	// sw
        instruction->signals.aluop  = ADD;
        instruction->signals.mw     = 1;
        instruction->signals.mr     = -1;
        instruction->signals.mtr    = 0;
        instruction->signals.asrc   = 1;
        instruction->signals.btype  = 00;
        instruction->signals.rdst   = -1;
        instruction->signals.rw     = 0;
        sprintf(instruction->string,"sw $%d, %d ($%d)",
                instruction->fields.rt, instruction->fields.imm, 
                instruction->fields.rs);
    } else if (is_bge) {    // bge
        instruction->signals.aluop  = SUB;
        instruction->signals.mw     = 0;
        instruction->signals.mr     = -1;
        instruction->signals.mtr    = 0;
        instruction->signals.asrc   = 0;
        instruction->signals.btype  = 10;
        instruction->signals.rdst   = -1;
        instruction->signals.rw     = 0;
        sprintf(instruction->string,"bge $%d, $%d, %d",
                instruction->fields.rs, instruction->fields.rt, 
                instruction->fields.imm);
    } else if (is_j) {      // j
        instruction->signals.aluop  = INV;
        instruction->signals.mw     = 0;
        instruction->signals.mr     = -1;
        instruction->signals.mtr    = 0;
        instruction->signals.asrc   = -1;
        instruction->signals.btype  = 01;
        instruction->signals.rdst   = -1;
        instruction->signals.rw     = 0;
        sprintf(instruction->string,"j %d",
                instruction->fields.imm);
    } else if (is_jal) {    // jal
        instruction->signals.aluop  = INV;
        instruction->signals.mw     = 0;
        instruction->signals.mr     = -1;
        instruction->signals.mtr    = 0;
        instruction->signals.asrc   = -1;
        instruction->signals.btype  = 01;
        instruction->signals.rdst   = -1;
        instruction->signals.rw     = 0;    // Register does not need to be written
        sprintf(instruction->string,"jal %d",
                instruction->fields.imm);
    }

	// fill in s1data and input2
    // Set the data up for executing
    switch (getFormat(instruction)) {
        case (R_format) :
            instruction->input1  = instruction->fields.rs;
            instruction->s1data  = regfile[instruction->fields.rs];
            instruction->input2  = instruction->fields.rt;
            instruction->s2data  = regfile[instruction->fields.rt];
            instruction->destreg = instruction->fields.rd;
            break;
        case (I_format) :
            instruction->input1  = instruction->fields.rs;
            instruction->s1data  = regfile[instruction->fields.rs];
            instruction->s2data  = regfile[instruction->fields.rt];
            instruction->destreg = instruction->fields.rt;
            break;

        // No need to do anything for J_format instructions
    }
}

/* execute
 *
 * This fills in the aluout value into the instruction and destdata
 */

void execute(InstInfo *instruction)
{
    int in1 = instruction->s1data;
    int in2 = (getFormat(instruction) == I_format) ? instruction->fields.imm : 
                                                     instruction->s2data;
    switch (instruction->signals.aluop) {
        case INV:   // j or jal
            break;      // Don't do anything
        case AND:
            instruction->aluout = in1 & in2;
            // printf("ALUOUT = %d s1 = %d s2 = %d\n", instruction->aluout, in1, in2);
            break;
        case OR:
            instruction->aluout = in1 | in2;
            break;
        case ADD:
            instruction->aluout = in1 + in2;
            break;
        case SUB:
            instruction->aluout = in1 - in2;
            break;
        case NOT:
            instruction->aluout = ~in1;
            break;
        case XOR:
            instruction->aluout = in1 ^ in2;
            break;
        case SLT:
            instruction->aluout = in1 < in2;
            break; 
    }
    instruction->destdata = instruction->aluout;
    
    // TODO: do the following using btype
    if (is_bge)         setPCWithInfo(BGE, instruction->aluout, instruction->fields.imm);
    else if (is_jal)    setPCWithInfo(JAL, instruction->aluout, instruction->fields.imm);
    else if (is_j)      setPCWithInfo(J,   instruction->aluout, instruction->fields.imm);
}

/* memory
 *
 * If this is a load or a store, perform the memory operation
 */
void memory(InstInfo *instruction)
{
    // Does the instruction read memory?
    if (instruction->signals.mr == 1) {
		instruction->memout = datamem[instruction->aluout];
	}
    
    // Does the instruction write memory?
    if (instruction->signals.mw == 1) {
        datamem[instruction->aluout] = instruction->s2data;
    }
}

/* writeback
 *
 * If a register file is supposed to be written, write to it now
 */
void writeback(InstInfo *instruction)
{
    instruction->destreg = (instruction->signals.rdst == 1) ? instruction->fields.rd :
                            (instruction->signals.rdst == 0) ? instruction->fields.rt :
                              -1;
    if (instruction->signals.rw == 1) {  // Register is supposeed to be written
        if (instruction->destreg == -1) printf("ERROR in the simulator!\n"), exit(-1); // Total comma hack
        regfile[instruction->destreg] = instruction->aluout;
    }

}
	

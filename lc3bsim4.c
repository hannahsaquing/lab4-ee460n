/***************************************************************/
/*                                                             */
/*   LC-3b Simulator                                           */
/*                                                             */
/*   EE 460N                                                   */
/*   The University of Texas at Austin                         */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/***************************************************************/
/*                                                             */
/* Files:  ucode        Microprogram file                      */
/*         isaprogram   LC-3b machine language program file    */
/*                                                             */
/***************************************************************/

/***************************************************************/
/* These are the functions you'll have to write.               */
/***************************************************************/

void eval_micro_sequencer();
void cycle_memory();
void eval_bus_drivers();
void drive_bus();
void latch_datapath_values();

/***************************************************************/
/* A couple of useful definitions.                             */
/***************************************************************/
#define FALSE 0
#define TRUE  1

/***************************************************************/
/* Use this to avoid overflowing 16 bits on the bus.           */
/***************************************************************/
#define Low16bits(x) ((x) & 0xFFFF)

/***************************************************************/
/* Definition of the control store layout.                     */
/***************************************************************/
#define CONTROL_STORE_ROWS 64
#define INITIAL_STATE_NUMBER 18

/***************************************************************/
/* Definition of bit order in control store word.              */
/***************************************************************/
enum CS_BITS {                                                  
    IRD,
    COND1, COND0,
    J5, J4, J3, J2, J1, J0,
    LD_MAR,
    LD_MDR,
    LD_IR,
    LD_BEN,
    LD_REG,
    LD_CC,
    LD_PC,
    GATE_PC,
    GATE_MDR,
    GATE_ALU,
    GATE_MARMUX,
    GATE_SHF,
    PCMUX1, PCMUX0,
    DRMUX,
    SR1MUX,
    ADDR1MUX,
    ADDR2MUX1, ADDR2MUX0,
    MARMUX,
    ALUK1, ALUK0,
    MIO_EN,
    R_W,
    DATA_SIZE,
    LSHF1,
/* MODIFY: you have to add all your new control signals */
    CONTROL_STORE_BITS
} CS_BITS;

/***************************************************************/
/* Functions to get at the control bits.                       */
/***************************************************************/
int GetIRD(int *x)           { return(x[IRD]); }
int GetCOND(int *x)          { return((x[COND1] << 1) + x[COND0]); }
int GetJ(int *x)             { return((x[J5] << 5) + (x[J4] << 4) +
				      (x[J3] << 3) + (x[J2] << 2) +
				      (x[J1] << 1) + x[J0]); }
int GetLD_MAR(int *x)        { return(x[LD_MAR]); }
int GetLD_MDR(int *x)        { return(x[LD_MDR]); }
int GetLD_IR(int *x)         { return(x[LD_IR]); }
int GetLD_BEN(int *x)        { return(x[LD_BEN]); }
int GetLD_REG(int *x)        { return(x[LD_REG]); }
int GetLD_CC(int *x)         { return(x[LD_CC]); }
int GetLD_PC(int *x)         { return(x[LD_PC]); }
int GetGATE_PC(int *x)       { return(x[GATE_PC]); }
int GetGATE_MDR(int *x)      { return(x[GATE_MDR]); }
int GetGATE_ALU(int *x)      { return(x[GATE_ALU]); }
int GetGATE_MARMUX(int *x)   { return(x[GATE_MARMUX]); }
int GetGATE_SHF(int *x)      { return(x[GATE_SHF]); }
int GetPCMUX(int *x)         { return((x[PCMUX1] << 1) + x[PCMUX0]); }
int GetDRMUX(int *x)         { return(x[DRMUX]); }
int GetSR1MUX(int *x)        { return(x[SR1MUX]); }
int GetADDR1MUX(int *x)      { return(x[ADDR1MUX]); }
int GetADDR2MUX(int *x)      { return((x[ADDR2MUX1] << 1) + x[ADDR2MUX0]); }
int GetMARMUX(int *x)        { return(x[MARMUX]); }
int GetALUK(int *x)          { return((x[ALUK1] << 1) + x[ALUK0]); }
int GetMIO_EN(int *x)        { return(x[MIO_EN]); }
int GetR_W(int *x)           { return(x[R_W]); }
int GetDATA_SIZE(int *x)     { return(x[DATA_SIZE]); } 
int GetLSHF1(int *x)         { return(x[LSHF1]); }
/* MODIFY: you can add more Get functions for your new control signals */

/***************************************************************/
/* The control store rom.                                      */
/***************************************************************/
int CONTROL_STORE[CONTROL_STORE_ROWS][CONTROL_STORE_BITS];

/***************************************************************/
/* Main memory.                                                */
/***************************************************************/
/* MEMORY[A][0] stores the least significant byte of word at word address A
   MEMORY[A][1] stores the most significant byte of word at word address A 
   There are two write enable signals, one for each byte. WE0 is used for 
   the least significant byte of a word. WE1 is used for the most significant 
   byte of a word. */

#define WORDS_IN_MEM    0x08000 
#define MEM_CYCLES      5
int MEMORY[WORDS_IN_MEM][2];

/***************************************************************/

/***************************************************************/

/***************************************************************/
/* LC-3b State info.                                           */
/***************************************************************/
#define LC_3b_REGS 8

int RUN_BIT;	/* run bit */
int BUS;	/* value of the bus */

typedef struct System_Latches_Struct{

int PC,		/* program counter */
    MDR,	/* memory data register */
    MAR,	/* memory address register */
    IR,		/* instruction register */
    N,		/* n condition bit */
    Z,		/* z condition bit */
    P,		/* p condition bit */
    BEN;        /* ben register */

int READY;	/* ready bit */
  /* The ready bit is also latched as you dont want the memory system to assert it 
     at a bad point in the cycle*/

int REGS[LC_3b_REGS]; /* register file. */

int MICROINSTRUCTION[CONTROL_STORE_BITS]; /* The microintruction */

int STATE_NUMBER; /* Current State Number - Provided for debugging */ 

/* For lab 4 */
int INTV; /* Interrupt vector register */
int EXCV; /* Exception vector register */
int SSP; /* Initial value of system stack pointer */
/* MODIFY: You may add system latches that are required by your implementation */

} System_Latches;

/* Data Structure for Latch */

System_Latches CURRENT_LATCHES, NEXT_LATCHES;

/***************************************************************/
/* A cycle counter.                                            */
/***************************************************************/
int CYCLE_COUNT;

/***************************************************************/
/*                                                             */
/* Procedure : help                                            */
/*                                                             */
/* Purpose   : Print out a list of commands.                   */
/*                                                             */
/***************************************************************/
void help() {                                                    
    printf("----------------LC-3bSIM Help-------------------------\n");
    printf("go               -  run program to completion       \n");
    printf("run n            -  execute program for n cycles    \n");
    printf("mdump low high   -  dump memory from low to high    \n");
    printf("rdump            -  dump the register & bus values  \n");
    printf("?                -  display this help menu          \n");
    printf("quit             -  exit the program                \n\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : cycle                                           */
/*                                                             */
/* Purpose   : Execute a cycle                                 */
/*                                                             */
/***************************************************************/
void cycle() {                                                

  eval_micro_sequencer();   
  cycle_memory();
  eval_bus_drivers();
  drive_bus();
  latch_datapath_values();

  CURRENT_LATCHES = NEXT_LATCHES;

  CYCLE_COUNT++;
}

/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate the LC-3b for n cycles.                 */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {                                      
    int i;

    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating for %d cycles...\n\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
	if (CURRENT_LATCHES.PC == 0x0000) {
	    RUN_BIT = FALSE;
	    printf("Simulator halted\n\n");
	    break;
	}
	cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate the LC-3b until HALTed.                 */
/*                                                             */
/***************************************************************/
void go() {                                                     
    if (RUN_BIT == FALSE) {
	printf("Can't simulate, Simulator is halted\n\n");
	return;
    }

    printf("Simulating...\n\n");
    while (CURRENT_LATCHES.PC != 0x0000)
	cycle();
    RUN_BIT = FALSE;
    printf("Simulator halted\n\n");
}

/***************************************************************/ 
/*                                                             */
/* Procedure : mdump                                           */
/*                                                             */
/* Purpose   : Dump a word-aligned region of memory to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void mdump(FILE * dumpsim_file, int start, int stop) {          
    int address; /* this is a byte address */

    printf("\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    printf("-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	printf("  0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    printf("\n");

    /* dump the memory contents into the dumpsim file */
    fprintf(dumpsim_file, "\nMemory content [0x%0.4x..0x%0.4x] :\n", start, stop);
    fprintf(dumpsim_file, "-------------------------------------\n");
    for (address = (start >> 1); address <= (stop >> 1); address++)
	fprintf(dumpsim_file, " 0x%0.4x (%d) : 0x%0.2x%0.2x\n", address << 1, address << 1, MEMORY[address][1], MEMORY[address][0]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */   
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump(FILE * dumpsim_file) {                               
    int k; 

    printf("\nCurrent register/bus values :\n");
    printf("-------------------------------------\n");
    printf("Cycle Count  : %d\n", CYCLE_COUNT);
    printf("PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    printf("IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    printf("STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    printf("BUS          : 0x%0.4x\n", BUS);
    printf("MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    printf("MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    printf("CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    printf("Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	printf("%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    printf("\n");

    /* dump the state information into the dumpsim file */
    fprintf(dumpsim_file, "\nCurrent register/bus values :\n");
    fprintf(dumpsim_file, "-------------------------------------\n");
    fprintf(dumpsim_file, "Cycle Count  : %d\n", CYCLE_COUNT);
    fprintf(dumpsim_file, "PC           : 0x%0.4x\n", CURRENT_LATCHES.PC);
    fprintf(dumpsim_file, "IR           : 0x%0.4x\n", CURRENT_LATCHES.IR);
    fprintf(dumpsim_file, "STATE_NUMBER : 0x%0.4x\n\n", CURRENT_LATCHES.STATE_NUMBER);
    fprintf(dumpsim_file, "BUS          : 0x%0.4x\n", BUS);
    fprintf(dumpsim_file, "MDR          : 0x%0.4x\n", CURRENT_LATCHES.MDR);
    fprintf(dumpsim_file, "MAR          : 0x%0.4x\n", CURRENT_LATCHES.MAR);
    fprintf(dumpsim_file, "CCs: N = %d  Z = %d  P = %d\n", CURRENT_LATCHES.N, CURRENT_LATCHES.Z, CURRENT_LATCHES.P);
    fprintf(dumpsim_file, "Registers:\n");
    for (k = 0; k < LC_3b_REGS; k++)
	fprintf(dumpsim_file, "%d: 0x%0.4x\n", k, CURRENT_LATCHES.REGS[k]);
    fprintf(dumpsim_file, "\n");
    fflush(dumpsim_file);
}

/***************************************************************/
/*                                                             */
/* Procedure : get_command                                     */
/*                                                             */
/* Purpose   : Read a command from standard input.             */  
/*                                                             */
/***************************************************************/
void get_command(FILE * dumpsim_file) {                         
    char buffer[20];
    int start, stop, cycles;

    printf("LC-3b-SIM> ");

    scanf("%s", buffer);
    printf("\n");

    switch(buffer[0]) {
    case 'G':
    case 'g':
	go();
	break;

    case 'M':
    case 'm':
	scanf("%i %i", &start, &stop);
	mdump(dumpsim_file, start, stop);
	break;

    case '?':
	help();
	break;
    case 'Q':
    case 'q':
	printf("Bye.\n");
	exit(0);

    case 'R':
    case 'r':
	if (buffer[1] == 'd' || buffer[1] == 'D')
	    rdump(dumpsim_file);
	else {
	    scanf("%d", &cycles);
	    run(cycles);
	}
	break;

    default:
	printf("Invalid Command\n");
	break;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : init_control_store                              */
/*                                                             */
/* Purpose   : Load microprogram into control store ROM        */ 
/*                                                             */
/***************************************************************/
void init_control_store(char *ucode_filename) {                 
    FILE *ucode;
    int i, j, index;
    char line[200];

    printf("Loading Control Store from file: %s\n", ucode_filename);

    /* Open the micro-code file. */
    if ((ucode = fopen(ucode_filename, "r")) == NULL) {
	printf("Error: Can't open micro-code file %s\n", ucode_filename);
	exit(-1);
    }

    /* Read a line for each row in the control store. */
    for(i = 0; i < CONTROL_STORE_ROWS; i++) {
	if (fscanf(ucode, "%[^\n]\n", line) == EOF) {
	    printf("Error: Too few lines (%d) in micro-code file: %s\n",
		   i, ucode_filename);
	    exit(-1);
	}

	/* Put in bits one at a time. */
	index = 0;

	for (j = 0; j < CONTROL_STORE_BITS; j++) {
	    /* Needs to find enough bits in line. */
	    if (line[index] == '\0') {
		printf("Error: Too few control bits in micro-code file: %s\nLine: %d\n",
		       ucode_filename, i);
		exit(-1);
	    }
	    if (line[index] != '0' && line[index] != '1') {
		printf("Error: Unknown value in micro-code file: %s\nLine: %d, Bit: %d\n",
		       ucode_filename, i, j);
		exit(-1);
	    }

	    /* Set the bit in the Control Store. */
	    CONTROL_STORE[i][j] = (line[index] == '0') ? 0:1;
	    index++;
	}

	/* Warn about extra bits in line. */
	if (line[index] != '\0')
	    printf("Warning: Extra bit(s) in control store file %s. Line: %d\n",
		   ucode_filename, i);
    }
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : init_memory                                     */
/*                                                             */
/* Purpose   : Zero out the memory array                       */
/*                                                             */
/***************************************************************/
void init_memory() {                                           
    int i;

    for (i=0; i < WORDS_IN_MEM; i++) {
	MEMORY[i][0] = 0;
	MEMORY[i][1] = 0;
    }
}

/**************************************************************/
/*                                                            */
/* Procedure : load_program                                   */
/*                                                            */
/* Purpose   : Load program and service routines into mem.    */
/*                                                            */
/**************************************************************/
void load_program(char *program_filename) {                   
    FILE * prog;
    int ii, word, program_base;

    /* Open program file. */
    prog = fopen(program_filename, "r");
    if (prog == NULL) {
	printf("Error: Can't open program file %s\n", program_filename);
	exit(-1);
    }

    /* Read in the program. */
    if (fscanf(prog, "%x\n", &word) != EOF)
	program_base = word >> 1;
    else {
	printf("Error: Program file is empty\n");
	exit(-1);
    }

    ii = 0;
    while (fscanf(prog, "%x\n", &word) != EOF) {
	/* Make sure it fits. */
	if (program_base + ii >= WORDS_IN_MEM) {
	    printf("Error: Program file %s is too long to fit in memory. %x\n",
		   program_filename, ii);
	    exit(-1);
	}

	/* Write the word to memory array. */
	MEMORY[program_base + ii][0] = word & 0x00FF;
	MEMORY[program_base + ii][1] = (word >> 8) & 0x00FF;
	ii++;
    }

    if (CURRENT_LATCHES.PC == 0) CURRENT_LATCHES.PC = (program_base << 1);

    printf("Read %d words from program into memory.\n\n", ii);
}

/***************************************************************/
/*                                                             */
/* Procedure : initialize                                      */
/*                                                             */
/* Purpose   : Load microprogram and machine language program  */ 
/*             and set up initial state of the machine.        */
/*                                                             */
/***************************************************************/
void initialize(char *argv[], int num_prog_files) { 
    int i;
    init_control_store(argv[1]);

    init_memory();
    for ( i = 0; i < num_prog_files; i++ ) {
	load_program(argv[i + 2]);
    }
    CURRENT_LATCHES.Z = 1;
    CURRENT_LATCHES.STATE_NUMBER = INITIAL_STATE_NUMBER;
    memcpy(CURRENT_LATCHES.MICROINSTRUCTION, CONTROL_STORE[INITIAL_STATE_NUMBER], sizeof(int)*CONTROL_STORE_BITS);
    CURRENT_LATCHES.SSP = 0x3000; /* Initial value of system stack pointer */

    NEXT_LATCHES = CURRENT_LATCHES;

    RUN_BIT = TRUE;
}

/***************************************************************/
/*                                                             */
/* Procedure : main                                            */
/*                                                             */
/***************************************************************/
int main(int argc, char *argv[]) {                              
    FILE * dumpsim_file;

    /* Error Checking */
    if (argc < 3) {
	printf("Error: usage: %s <micro_code_file> <program_file_1> <program_file_2> ...\n",
	       argv[0]);
	exit(1);
    }

    printf("LC-3b Simulator\n\n");

    initialize(argv, argc - 2);

    if ( (dumpsim_file = fopen( "dumpsim", "w" )) == NULL ) {
	printf("Error: Can't open dumpsim file\n");
	exit(-1);
    }

    while (1)
	get_command(dumpsim_file);

}

/***************************************************************/
/* Do not modify the above code, except for the places indicated 
   with a "MODIFY:" comment.

   Do not modify the rdump and mdump functions.

   You are allowed to use the following global variables in your
   code. These are defined above.

   CONTROL_STORE
   MEMORY
   BUS

   CURRENT_LATCHES
   NEXT_LATCHES

   You may define your own local/global variables and functions.
   You may use the functions to get at the control bits defined
   above.

   Begin your code here 	  			       */
/***************************************************************/


#define Low8bits(x) ((x) & 0x00FF)
#define getHighByte(x) (((x) & 0x00FF) << 8) 
#define writeHighByte(x) (((x) >> 8))

int getRegNum (int place) {
    return ((CURRENT_LATCHES.IR >> place) & 0b00000111);
}

int SignExtend(int value, int bits){ // 5, 9, 11, 6, 8 -- taken from lab2
    switch (bits) {
      case 5: 
       if (0x00000010 & value){ 
        // number is negative
        return Low16bits(value | 0xFFE0); // 1111 1111 1110 0000

       }else{
        //number is positive
        return Low16bits(value & 0x001F); // 0001 1111 -- need 6 bits tho
       }
      break;

      case 9: 
       if (0x00000100 & value){ // 0000 0000 0000 0000 0000 0001 0000 0000
        // number is negative
        return Low16bits(value | 0xFE00); // 1111 1110 0000 0000

       }else{
        //number is positive
        return Low16bits(value & ~0xFE00); // 0000 0001 1111 1111
       }
      break;

      case 8:
      if (0x00000080 & value){ // 0000 0000 0000 0000 0000 0000 1000 0000
        // number is negative
        return Low16bits(value | 0xFF00); 

       }else{
        //number is positive
        return Low16bits(value & ~0xFF00);
       }
      break;

      case 11: 
      if (0x00000400 & value){ // 0000 0000 0000 0000 0100 0000 0000
        // number is negative
        return Low16bits(value | 0xF800); 

       }else{
        //number is positive
        return Low16bits(value & ~0xF800);
       }

      break;

      case 14: 
      if (0x0002000 & value){ // 0000 0000 0000 0010 0000 0000 0000
        // number is negative
        return Low16bits(value | 0xC800);  // 1100 0000 0000 0000

       }else{
        //number is positive
        return Low16bits(value & ~0xC800);
       }

      break;

      case 6:
      if (0x0000020 & value){ // 0010 0000
        // number is negative
        return Low16bits(value | 0xFFC0); 

       }else{
        //number is positive
        return Low16bits(value & ~0xFFC0);
       }
      break;
    }
}

int getOpcode(int instruction) {
    int opcode = instruction >> 12;
    opcode &= 0x0F; // 0000 0000 0000 1111
    return opcode;
}

void setBEN (int instruction) {
    int ir11, ir10, ir9;
    ir11 = (instruction >> 11) & 0x0001;
    ir10 = (instruction >> 10) & 0x0001;
    ir9 = (instruction >> 9) & 0x0001;
    NEXT_LATCHES.BEN = (ir11 && CURRENT_LATCHES.N) || (ir10 && CURRENT_LATCHES.Z) || (ir9 && CURRENT_LATCHES.P);
}

void eval_micro_sequencer() {
    printf("Starting eval_micro_sequencer()...\n");
    //printf("CURRENT STATE:%.4x\n", CURRENT_LATCHES.STATE_NUMBER);

  /* 
   * Evaluate the address of the next state according to the 
   * micro sequencer logic. Latch the next microinstruction.
   */
  
  // See if you're going to BR
  int instr = GetIRD(CURRENT_LATCHES.MICROINSTRUCTION);
  if (instr) {
    NEXT_LATCHES.STATE_NUMBER = getOpcode(CURRENT_LATCHES.IR);
    //printf("GOT OPCODE\n");
  }
  else {
    //printf("Didn't get IRD...\n");
    NEXT_LATCHES.STATE_NUMBER = GetJ(CURRENT_LATCHES.MICROINSTRUCTION);

    if (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION) == 0b10 && CURRENT_LATCHES.BEN) {
        //printf("BEN ENABLED\n");
        NEXT_LATCHES.STATE_NUMBER |= 0x04;
    }
    if (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION) == 0b01 && CURRENT_LATCHES.READY) {
        //printf("READY BIT ASSERTED\n");
        NEXT_LATCHES.STATE_NUMBER |= 0x02;
    }
    if (GetCOND(CURRENT_LATCHES.MICROINSTRUCTION) == 0b11) {
        //printf("ADDR MODE\n");
        printf("IR = %.4x\n", CURRENT_LATCHES.IR);
        int ir11 = Low16bits(CURRENT_LATCHES.IR & 0x0800);
        ir11 = ir11 >> 10;
        printf("IR[11]: %d\n", ir11);
        if (ir11) {
            NEXT_LATCHES.STATE_NUMBER |= 0x01;
        }
        printf("Next State: %.4x\n", NEXT_LATCHES.STATE_NUMBER);
    }
  }  //printf("Headed to state %.4x\n", NEXT_LATCHES.STATE_NUMBER);
  int i;
  int numBits = CONTROL_STORE_BITS;
  for (i = 0; i < numBits; i++) {
    NEXT_LATCHES.MICROINSTRUCTION[i] = CONTROL_STORE[NEXT_LATCHES.STATE_NUMBER][i];
  }
  //printf("Next state: 0x%.4x\n", NEXT_LATCHES.STATE_NUMBER);
}
int men = 0;

void cycle_memory() {
    printf("Starting cycle_memory()...\n");
  /* 
   * This function emulates memory and the WE logic. 
   * Keep track of which cycle of MEMEN we are dealing with.  
   * If fourth, we need to latch Ready bit at the end of 
   * cycle to prepare microsequencer for the fifth cycle.  
   */

  int mio_en = GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION);
  if (mio_en) {
    men++;
  }
  else {
    printf("memory not enabled, returning...\n");
    return; // shouldn't mess with memory enable counter if the instr doesn't need mem
  }
  if (men < 4) {
    NEXT_LATCHES.READY = 0;
    //printf("men:%d\n\n", men);
  }
  else {
    //printf("Next Latch: READY\n");
    NEXT_LATCHES.READY = 1;
  }
  if (CURRENT_LATCHES.READY) {
    // change from word addressable to byte addressable
    //printf("Current latch READY\n");
    int addy = CURRENT_LATCHES.MAR / 2;
    // determine if read or write
    int rw = GetR_W(CURRENT_LATCHES.MICROINSTRUCTION);
    int read = 0;
    int write = 1;
    int address = CURRENT_LATCHES.MAR / 2;
    if (rw == read) {
        // do read (LDW, LDB, LEA?)
        //printf("READING...\n");
        NEXT_LATCHES.MDR = Low8bits(MEMORY[address][0]);
        NEXT_LATCHES.MDR |= (getHighByte(MEMORY[address][1]));
        /*if (!GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {
            if (CURRENT_LATCHES.MAR & 0x0001 == 1) {
                NEXT_LATCHES.MDR = NEXT_LATCHES.MDR >> 8;
                NEXT_LATCHES.MDR = NEXT_LATCHES.MDR & 0x00FF;
            }
            else {
                NEXT_LATCHES.MDR = NEXT_LATCHES.MDR & 0x00FF;
            }
            NEXT_LATCHES.MDR = SignExtend(NEXT_LATCHES.MDR, 8);
        }*/
    }
    else if (rw == write) {
        printf("WRITING!! We are writing from the data from address 0x%.4x\n", address);
        int word = GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION);
        if (word) {
            MEMORY[address][0] = Low8bits(CURRENT_LATCHES.MDR);
            MEMORY[address][1] = Low8bits(writeHighByte(CURRENT_LATCHES.MDR));
        }
        else { // otherwise we are writing a byte
            //int idx = 0x0001 & CURRENT_LATCHES.MAR;
            int idx = (CURRENT_LATCHES.MAR % 2) ? 1 : 0;
            MEMORY[address][idx] = Low8bits(CURRENT_LATCHES.MDR);
        }
    }

    NEXT_LATCHES.READY = 0;
    //printf("Next_latch READY set to 0\n");
    men = 0; //per usual
  }
}

int driveMM = 0; 
int drivePC = 0;
int driveALU = 0;
int driveSHF = 0;
int driveMDR = 0;

int Gate_MARMUX;
int Gate_PC;
int Gate_ALU;
int Gate_SHF;
int Gate_MDR;

int bit(int x, int y) {
  return (((x) & (1 << y)) >> y);} 

void eval_bus_drivers() {
    printf("Starting eval_bus_drivers\n");
    int origin, addition, regLocation, usePC;
  /* 
   * Datapath routine emulating operations before driving the bus.
   * Evaluate the input of tristate drivers 
   *         Gate_MARMUX,
   *		 Gate_PC,
   *		 Gate_ALU,
   *		 Gate_SHF,
   *		 Gate_MDR.
   */    
  if (GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION)) {
    //printf("Driving MARMAX...\n");
    driveMM = 1;
    //int marmux_1 = GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION); // select LSHF(ZEXT[IR[7:0]],1)
    //printf("MARMUX = %d\n", GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION));
    if (GetMARMUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0) {
        printf("Selected LSHF(ZEXT[IR[7:0]],1)\n");
        Gate_MARMUX = Low8bits(CURRENT_LATCHES.IR) << 1;
        //Gate_MARMUX = SignExtend(Low8bits(CURRENT_LATCHES.IR) << 1, 8);
    }
    else {
        if (GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0) {
            //printf("origin = current pc\n");
            origin = CURRENT_LATCHES.PC;
            //printf("Origin PC: 0x%.4x\n", origin);

        }
        else { // calculating base register
            // this is taking the data from memory not the address
            //origin = ((GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)) ? CURRENT_LATCHES.REGS[getRegNum(6)] : CURRENT_LATCHES.REGS[getRegNum(9)]);
            printf("GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION: %d\n", (GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)));
            if ((GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0)) { // ,source IR[11:9]
                printf("Base Register is R%d\n", getRegNum(9));
                origin = CURRENT_LATCHES.REGS[getRegNum(9)];
            }
            else { // source IR[8:6]
                printf("Base Register is R%d\n", getRegNum(6));                
                origin = CURRENT_LATCHES.REGS[getRegNum(6)];
            }
            printf("Origin address from Base Register: 0x%.4x\n", origin);
        }
        printf("GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION): %d\n", GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION));
        int offset = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
        if (offset == 0b01) { // select SEXT[IR[5:0]]
            addition = SignExtend(CURRENT_LATCHES.IR, 6);
            printf("Offset6: %d\n", addition);
        }    
        else if (offset == 0b10) { // select SEXT[IR[8:0]]
            addition = SignExtend(CURRENT_LATCHES.IR, 9);
            printf("Offset9: %d\n", addition);
            }
        else if (offset == 0b11) { // select SEXT[IR[10:0]]
            addition = SignExtend(CURRENT_LATCHES.IR, 11);
            printf("Offset11: %d\n", addition);
            }
        else {printf("offset 0\n"); addition = 0;}
        //addition = addition * 2;
        //printf("GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION): %d\n", GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION));
        printf("before shift Origin: %.4x\nAddition: %.4x\n", origin, addition);
        addition = (GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)) ? addition << 1 : addition;
        printf("Origin: %.4x\nAddition: %.4x\n", origin, addition);
        Gate_MARMUX = origin + addition;
        printf("Gate_MARMUX is = 0x%.4x\n", Gate_MARMUX);
        //printf("Gate_MARMUX = 0x%.4x, Origin = 0x%.4x, Addition = %d\n", Gate_MARMUX, origin, addition);

    }
  }  
  else {
    driveMM = 0;
  }
  
  if (GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION)) {
    drivePC = 1;
    Gate_PC = CURRENT_LATCHES.PC;
  } else {drivePC = 0;}
   
   if (GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION)) {
    printf("Gating ALU...\n");
    driveALU = 1;
    int sr1 = (GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 0) ? CURRENT_LATCHES.REGS[getRegNum(9)] : CURRENT_LATCHES.REGS[getRegNum(6)]; 
    printf("sr1 = 0x%.4x\n", sr1);
    int sr2 = (CURRENT_LATCHES.IR & 0x0020) ? SignExtend(CURRENT_LATCHES.IR, 5) : CURRENT_LATCHES.REGS[getRegNum(0)];
    printf("sr2 = 0x%.4x\n", sr2);    
    int op = GetALUK(CURRENT_LATCHES.MICROINSTRUCTION);
    printf("ALUK = %d\n", op);
    if (op == 0) {
        Gate_ALU = sr1 + sr2;
    }
    else if (op == 1) {
        Gate_ALU = sr1 & sr2;
    } 
    else if (op == 2) { 
        Gate_ALU = sr1 ^ sr2;
    }
    else if (op == 3) {
        Gate_ALU = sr1;
    }
    int byte = 0;
    if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) == byte && (GetJ(CURRENT_LATCHES.MICROINSTRUCTION) == 24)) {
        int newVal = Gate_ALU & 0x00FF;
        Gate_ALU = Low8bits(newVal);
    }
    printf("Gate_ALU Value: 0x%.4x\n", Gate_ALU);
 } else {driveALU = 0;}
  
  if (GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION)) {
    driveSHF = 1;
    int sr1 = (GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION) == 1) ? CURRENT_LATCHES.REGS[getRegNum(6)] : CURRENT_LATCHES.REGS[getRegNum(9)];
    printf("SR1 = %.4x\n", sr1);
    int shfAmt = Low8bits(CURRENT_LATCHES.IR) & 0x0F;
    printf("Shift Amount = %d\n", shfAmt);
    int shiftType = (CURRENT_LATCHES.IR >> 4) & 0x0003;
    printf("Shift Type = %d\n", shiftType);
    int val = 0;
    if (shiftType == 0b00) { //left shift logical
        val = (sr1 << shfAmt);
    }
    else if (shiftType == 0b01) { // right shift logical
        val = (sr1 >> shfAmt);
    }
    else if (shiftType == 0b11) {
        val = (sr1 >> shfAmt);
        int signBit = (sr1 & 0x8000) >> 15;
        printf("Sign Bit = %d\n", signBit);
        if (signBit) {
            //int onesToShiftIn = (16 - shfAmt);     
            int orVal = 0xFFFF >> shfAmt;
            printf("orVal before not: %.4x\n", orVal);
            orVal = Low16bits(~orVal);
            printf("orVal: %.4x\nval: %.4x\n", orVal, val);
            val = val | orVal;
            val = Low16bits(val);
        }
        /*int i = 0;
        for(i = 0; i< shfAmt;i++){
          Gate_SHF = (!bit(CURRENT_LATCHES.REGS[sr1],15)) ? Low16bits(CURRENT_LATCHES.REGS[sr1])/2 : (0xFFFF0000 | CURRENT_LATCHES.REGS[sr1])/2;
        }*/
    }
    Gate_SHF = Low16bits(val);
    printf("Shifted Value: 0x%.4x\n", Gate_SHF);
  } else {driveSHF = 0;}
  
  if (GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION)) {
        printf("GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION) Activated\nLatching %.4x to Gate_MDR\n", CURRENT_LATCHES.MDR);
        driveMDR = 1;
        Gate_MDR = CURRENT_LATCHES.MDR;        
            if (!GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) {
                if (CURRENT_LATCHES.MAR & 0x0001 == 1) {
                    Gate_MDR = Gate_MDR >> 8;
                    Gate_MDR = Gate_MDR & 0x00FF;
                }
                else {
                    Gate_MDR = Gate_MDR & 0x00FF;
                }
                Gate_MDR = SignExtend(Gate_MDR, 8);
            }
  } else {driveMDR = 0;}

    Gate_MARMUX = Low16bits(Gate_MARMUX);
    Gate_ALU = Low16bits(Gate_ALU);
    Gate_PC = Low16bits(Gate_PC);
    Gate_SHF = Low16bits(Gate_SHF);
    Gate_MDR = Low16bits(Gate_MDR);

    //printf("GetGATE_MARMUX: %d\n", GetGATE_MARMUX(CURRENT_LATCHES.MICROINSTRUCTION));
    //printf("GetGATE_ALU: %d\n", GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION));
    /*printf("GetGATE_PC: %d\n", GetGATE_PC(CURRENT_LATCHES.MICROINSTRUCTION));
    printf("GetGATE_SHF: %d\n", GetGATE_SHF(CURRENT_LATCHES.MICROINSTRUCTION));
    printf("GetGATE_MDR: %d\n", GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION));
    printf("Gate_MARMUX: 0x%.4x\n", Gate_MARMUX);
    printf("Gate_ALU: 0x%.4x\n", Gate_ALU);
    printf("Gate_PC: 0x%.4x\n", Gate_PC);
    printf("Gate_SHF: 0x%.4x\n", Gate_SHF);
    printf("Gate_MDR: 0x%.4x\n", Gate_MDR);*/
}
  void drive_bus() {
    printf("Starting drive_bus()...\n");
    
  /* 
   * Datapath routine for driving the bus from one of the 5 possible 
   * tristate drivers. 
   */       
    //printf("driveMM = %d, drivePC = %d, driveALU = %d, driveSHF = %d, driveMDR = %d\n", driveMM, drivePC, driveALU, driveSHF, driveMDR);

    int origin, addition, regLocation, usePC;
    if (driveMM) {
        printf("Gating MARMUX...\n");
        BUS = Gate_MARMUX;
    } 
    else if (drivePC) {
        printf("Gating PC...\n");
        BUS = Gate_PC;
    }
    else if (driveALU) {
        printf("Gating ALU...\n");
        BUS = Gate_ALU;
    }
    else if (driveSHF) {
        printf("Gating SHF... value is 0x%.4x\n", Gate_SHF);
        BUS = Gate_SHF;
    }
    else if (driveMDR) {
        printf("Gating MDR...\n");
        BUS = Gate_MDR;
    }
    else {
        BUS = 0;
    }
}


void latch_datapath_values() {
    printf("Starting latch_datapath_values()...\n\n");

  /* 
   * Datapath routine for computing all functions that need to latch
   * values in the data path at the end of this cycle.  Some values
   * require sourcing the bus; therefore, this routine has to come 
   * after drive_bus.
   * 
   *  LD.MAR	 LD.MDR	 LD.IR	 LD.BEN	 LD.REG	 LD.CC	 LD.PC

   */       
  if (GetLD_MAR(CURRENT_LATCHES.MICROINSTRUCTION)) {
    NEXT_LATCHES.MAR = Low16bits(BUS);
  }
  
  if (GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION) && GetMIO_EN(CURRENT_LATCHES.MICROINSTRUCTION) && CURRENT_LATCHES.READY) {
    if (GetGATE_MDR(CURRENT_LATCHES.MICROINSTRUCTION)) {
        NEXT_LATCHES.MDR = (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION)) ? BUS : ((CURRENT_LATCHES.MDR % 2)) ? Low8bits(BUS) : getHighByte(BUS);
    }
    else {
        int address = (CURRENT_LATCHES.MAR / 2);
        NEXT_LATCHES.MDR = Low8bits(MEMORY[address][0]);
        NEXT_LATCHES.MDR |= (getHighByte(MEMORY[address][1]));
    }
  }

  else if (GetLD_MDR(CURRENT_LATCHES.MICROINSTRUCTION) && GetGATE_ALU(CURRENT_LATCHES.MICROINSTRUCTION)) {
        if (GetDATA_SIZE(CURRENT_LATCHES.MICROINSTRUCTION) == 1) {
            NEXT_LATCHES.MDR = Low16bits(BUS);
        }
        else {
            NEXT_LATCHES.MDR = Low8bits(BUS);
        }
    }

  if (GetLD_IR(CURRENT_LATCHES.MICROINSTRUCTION)) {
    NEXT_LATCHES.IR = Low16bits(BUS);
  }

  if (GetLD_BEN(CURRENT_LATCHES.MICROINSTRUCTION)) {
    int ir11, ir10, ir9;
    int ir = CURRENT_LATCHES.IR;
    ir11 = ( ir >> 11) & 0x0001;
    ir10 = ( ir >> 10) & 0x0001;
    ir9 = (  ir >> 9) & 0x0001;
    NEXT_LATCHES.BEN = (ir11 && CURRENT_LATCHES.N) || (ir10 && CURRENT_LATCHES.Z) || (ir9 && CURRENT_LATCHES.P);
  }

    //printf("GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION): %d\n", GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION));
    //printf("GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION): %d\n", GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION));
    if (GetLD_REG(CURRENT_LATCHES.MICROINSTRUCTION)) {
    if (GetDRMUX(CURRENT_LATCHES.MICROINSTRUCTION)) {
        //printf("Low16bits(BUS): %d", Low16bits(BUS));
        NEXT_LATCHES.REGS[7] = Low16bits(BUS);
    } 
    else {
        printf("BUS VALUE: %.4x\n", BUS);
        printf("Register #%d\n", getRegNum(9));
        NEXT_LATCHES.REGS[getRegNum(9)] = Low16bits(BUS);
    }
  }

  if (GetLD_CC(CURRENT_LATCHES.MICROINSTRUCTION)) {
    NEXT_LATCHES.N = 0;
    NEXT_LATCHES.Z = 0;
    NEXT_LATCHES.P = 0;
    int x = BUS;
    if(x==0){
        NEXT_LATCHES.Z = 1;
    } 
    else if(!bit(x, 15)){
        NEXT_LATCHES.P = 1;
    } 
    else if(bit(x, 15)){
        NEXT_LATCHES.N = 1;
    } 
  }

  if (GetLD_PC(CURRENT_LATCHES.MICROINSTRUCTION)) {
    int pcMux = GetPCMUX(CURRENT_LATCHES.MICROINSTRUCTION);
    if (pcMux == 0b00) { // PC + 2
        NEXT_LATCHES.PC = Low16bits(CURRENT_LATCHES.PC + 2);
    } 
    else if (pcMux == 0b01) { // bus
        NEXT_LATCHES.PC = Low16bits(BUS);
    } 
    else if (pcMux == 0b10) { // output of address adder
        int origin, addition;
        if (GetADDR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)) {
            origin = (GetSR1MUX(CURRENT_LATCHES.MICROINSTRUCTION)) ? CURRENT_LATCHES.REGS[getRegNum(6)] : CURRENT_LATCHES.REGS[getRegNum(9)];      
        }
        else {
            origin = CURRENT_LATCHES.PC;
        }
        int addr2mux = GetADDR2MUX(CURRENT_LATCHES.MICROINSTRUCTION);
        if (addr2mux == 0) { // zero
            addition = 0;
        }
        else if (addr2mux == 1) { // offset6
            addition = SignExtend(CURRENT_LATCHES.IR, 6);
        }
        else if (addr2mux == 2) { //offset9
            addition = SignExtend(CURRENT_LATCHES.IR, 9);
            printf("Addition = %d\n", addition);
        }
        else if (addr2mux == 3) { //offset9
            addition = SignExtend(CURRENT_LATCHES.IR, 11);
        }
        else {
            addition = 0;
        }
        addition = (GetLSHF1(CURRENT_LATCHES.MICROINSTRUCTION)) ? addition << 1 : addition;   
        NEXT_LATCHES.PC = origin + addition; 
        NEXT_LATCHES.PC = Low16bits(NEXT_LATCHES.PC);
        }
    }
}
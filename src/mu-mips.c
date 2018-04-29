#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"
#include "mu-cache.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("forwarding\t-- Enable or disable data forwarding in the pipeline\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	//Clear IF_ID
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;ID_EX = IF_ID;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
		case 'F':
		case 'f':
			if(scanf("%d", &ENABLE_FORWARDING) != 1){
				break;
			}
			ENABLE_FORWARDING == 0 ? printf("Forwarding OFF\n") : printf("Forwarding ON\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */
	
	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	uint32_t rd, rt, rs;
	rd = MEM_WB.IR & 0xF800;
	rd >>= 11; 
	rt = MEM_WB.IR & 0x1F0000;
	rt >>= 16;
	rs = MEM_WB.IR & 0x3E00000;
	rs >>= 21;
	
	if(MEM_WB.memory_reference_load){
    printf("WB_MEMWB DEST: %x    MEMWB LMD: %x",MEM_WB.destination, MEM_WB.LMD);
		NEXT_STATE.REGS[MEM_WB.destination] = MEM_WB.LMD;
    printf("WB_NEXT STATE REG VALUE: %x", NEXT_STATE.REGS[MEM_WB.destination]);
	}
	if(MEM_WB.register_register){
		NEXT_STATE.REGS[MEM_WB.destination] = MEM_WB.ALUOutput;
	}
	if(MEM_WB.register_immediate){
		NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
	}
	if(MEM_WB.MFHI){
		NEXT_STATE.REGS[MEM_WB.destination] = CURRENT_STATE.HI;
	}
	if(MEM_WB.MTHI){
		NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
	}
	if(MEM_WB.MFLO){
		NEXT_STATE.REGS[MEM_WB.destination] = CURRENT_STATE.LO;
	}
	if(MEM_WB.MTLO){
		NEXT_STATE.LO = CURRENT_STATE.REGS[rs];
	}
	if(MEM_WB.MULDIV){
		NEXT_STATE.LO = MEM_WB.LO;
		NEXT_STATE.HI = MEM_WB.HI;
	}
	
	INSTRUCTION_COUNT++;
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
  
  if(!(EX_MEM.memory_reference_load || EX_MEM.memory_reference_store)){
    return;
  }
  uint32_t currentTag = MEM_WB.ALUOutput & 0xFFFFF000;
  uint32_t byteOffset = MEM_WB.ALUOutput & 0x3;
  uint32_t wordOffset = MEM_WB.ALUOutput & 0xC;
  uint32_t blockIndex = MEM_WB.ALUOutput & 0xF0;
  uint32_t blockAddress = MEM_WB.ALUOutput & 0xFFFFFFFC;
  if(cacheStalling==0){
    //not stalling
    MEM_WB = EX_MEM;
	  memset(&EX_MEM, 0, sizeof(EX_MEM)); //Clear EX_MEM
    uint32_t currentTag = MEM_WB.ALUOutput & 0xFFFFF000;
    uint32_t byteOffset = MEM_WB.ALUOutput & 0x3;
    uint32_t wordOffset = MEM_WB.ALUOutput & 0xC;
    uint32_t blockIndex = MEM_WB.ALUOutput & 0xF0;
    uint32_t blockAddress = MEM_WB.ALUOutput & 0xFFFFFFFC;
    
    printf("\nCACHE is not stalling!");
    CacheBlock* currentBlock;
    currentBlock = &L1Cache.blocks[blockIndex];
    printf("\nAfter block set");
    if((currentBlock->tag == currentTag) && (currentBlock->valid == 1)){
      printf("\nCACHE Hit!");
      //cache hit, so load/store from cache
      cache_hits++;
      if(MEM_WB.memory_reference_load){
        printf("\nCACHE Memory Load");
       MEM_WB.LMD = currentBlock->words[wordOffset];
      printf("\nCACHE_MEMWB LMD: %x", MEM_WB.LMD);
      } else if(MEM_WB.memory_reference_store){
        printf("\nCACHE Memory Store");
        currentBlock->words[wordOffset] = MEM_WB.B; //update cache
        writeBuffer = *currentBlock; //put cache block into write buffer
        writeBufferToMemory(blockAddress); //write write buffer to memory
      }
      
    } else {
      printf("\nCACHE Miss!");
      //cache miss, start stalling
      cacheStalling++;
      cache_misses++;
    }
    
  } else {
    //cache stalling
    if(cacheStalling == 100){
      //end of cache stalling
      printf("\nEnd of stalling! memLoad: %d   memStore: %d", MEM_WB.memory_reference_load, MEM_WB.memory_reference_store);
      cacheStalling = 0;
      stalling = 0;
      if(MEM_WB.memory_reference_load){
        printf("\nCACHE Memory Load");
        //read all words in block and place each into cache
        L1Cache.blocks[blockIndex].words[0] = mem_read_32(blockAddress);
        L1Cache.blocks[blockIndex].words[1] = mem_read_32(blockAddress+0x4);
        L1Cache.blocks[blockIndex].words[2] = mem_read_32(blockAddress+0x8);
        L1Cache.blocks[blockIndex].words[3] = mem_read_32(blockAddress+0xC);
        L1Cache.blocks[blockIndex].valid = 1; //block is now valid
        
        MEM_WB.LMD = L1Cache.blocks[blockIndex].words[wordOffset]; //return word to CPU
        printf("\nCACHE_MEMWB LMD: %x", MEM_WB.LMD);

      } else if(MEM_WB.memory_reference_store){
         printf("\nCACHE Memory Store");
         //read all words in block and place each into cache
        L1Cache.blocks[blockIndex].words[0] = mem_read_32(blockAddress);
        L1Cache.blocks[blockIndex].words[1] = mem_read_32(blockAddress+0x4);
        L1Cache.blocks[blockIndex].words[2] = mem_read_32(blockAddress+0x8);
        L1Cache.blocks[blockIndex].words[3] = mem_read_32(blockAddress+0xC);
        L1Cache.blocks[blockIndex].valid = 1; //block is now valid
        L1Cache.blocks[blockIndex].words[wordOffset] = MEM_WB.B; //update new word in cache
        
        //place updated cache block in write buffer
        writeBuffer.words[0] = L1Cache.blocks[blockIndex].words[0];
        writeBuffer.words[1] = L1Cache.blocks[blockIndex].words[1];
        writeBuffer.words[2] = L1Cache.blocks[blockIndex].words[2];
        writeBuffer.words[3] = L1Cache.blocks[blockIndex].words[3];
        writeBufferToMemory(blockAddress);
      }
    } else {
      cacheStalling++;
    }
  }
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	
	uint64_t product, p1, p2;
	EX_MEM = ID_EX;
	memset(&ID_EX, 0, sizeof(ID_EX)); //Clear ID_EX
	EX_MEM.memory_reference_load = 0;
	EX_MEM.memory_reference_store = 0;
	EX_MEM.register_register = 0;
	EX_MEM.register_immediate = 0;
	EX_MEM.MFHI = 0; 
	EX_MEM.MTHI = 0; 
	EX_MEM.MFLO = 0;
	EX_MEM.MTLO = 0;
	EX_MEM.MULDIV = 0;
	EX_MEM.RegWrite = 0;

	if(EX_MEM.opcode == 0x00 && EX_MEM.IR != 0x00){
		switch(EX_MEM.function){
				case 0x00:{ //SLL
					uint32_t sa = EX_MEM.imm & 0x07C0;
					sa = sa >> 6;
					EX_MEM.ALUOutput = EX_MEM.B << sa;

					EX_MEM.destination = EX_MEM.registerRd;
					
					EX_MEM.register_register = 1;
					break;
				}
				case 0x02:{ //SRL
					uint32_t sa = EX_MEM.imm & 0x07C0;
					sa = sa >> 6;
					EX_MEM.ALUOutput = EX_MEM.B >> sa;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				}
				case 0x03:{ //SRA 
					uint32_t sa = EX_MEM.imm & 0x07C0;
					sa = sa >> 6;
					if ((sa & 0x10) == 1)
					{
						EX_MEM.ALUOutput =  ~(~EX_MEM.B >> sa);
					}
					else{
						EX_MEM.ALUOutput = EX_MEM.B >> sa;
					}
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				}
				case 0x0C: //SYSCALL
					if(CURRENT_STATE.REGS[2] == 0xa){
						RUN_FLAG = FALSE;
					}
					break;
				case 0x10: //MFHI *******LOAD/STORE********* HI -> rd
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.MFHI = 1;
					break;
				case 0x11: //MTHI *******LOAD/STORE********* rs -> HI
					EX_MEM.destination = 32; //32 represents LO/HI registers as destination
					EX_MEM.MTHI = 1;
					break;
				case 0x12: //MFLO *******LOAD/STORE********* LO -> rd
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.MFLO = 1;
					break;
				case 0x13: //MTLORegWrite *******LOAD/STORE********* rs -> LO
					EX_MEM.destination = 32; //32 represents LO/HI registers as destination
					EX_MEM.MTLO = 1;
					break;
				case 0x18: //MULT
					if ((EX_MEM.A & 0x80000000) == 0x80000000){
						p1 = 0xFFFFFFFF00000000 | EX_MEM.A;
					}else{
						p1 = 0x00000000FFFFFFFF & EX_MEM.A;
					}
					if ((EX_MEM.B & 0x80000000) == 0x80000000){
						p2 = 0xFFFFFFFF00000000 | EX_MEM.B;
					}else{
						p2 = 0x00000000FFFFFFFF & EX_MEM.B;
					}
					product = p1 * p2;
					EX_MEM.LO = (product & 0X00000000FFFFFFFF);
					EX_MEM.HI = (product & 0XFFFFFFFF00000000)>>32;
					EX_MEM.destination = 32; //32 represents LO/HI registers as destination
					EX_MEM.MULDIV = 1;
					break;
				case 0x19: //MULTU
					product = (uint64_t)EX_MEM.A * (uint64_t)EX_MEM.B;
					EX_MEM.LO = (product & 0X00000000FFFFFFFF);
					EX_MEM.HI = (product & 0XFFFFFFFF00000000)>>32;
					EX_MEM.destination = 32; //32 represents LO/HI registers as destination
					EX_MEM.MULDIV = 1;
					break;
				case 0x1A: //DIV 
					if(EX_MEM.B != 0)
					{
						EX_MEM.LO = (int32_t)EX_MEM.A / (int32_t)EX_MEM.B;
						EX_MEM.HI = (int32_t)EX_MEM.A % (int32_t)EX_MEM.B;
					}
					EX_MEM.destination = 32; //32 represents LO/HI registers as destination
					EX_MEM.MULDIV = 1;
					break;
				case 0x1B: //DIVU
					if(EX_MEM.B != 0)
					{
						EX_MEM.LO = (int32_t)EX_MEM.A / (int32_t)EX_MEM.B;
						EX_MEM.HI = (int32_t)EX_MEM.A % (int32_t)EX_MEM.B;
					}
					EX_MEM.destination = 32; //32 represents LO/HI registers as destination
					EX_MEM.MULDIV = 1;
					break;
				case 0x20: //ADD
					EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.B;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x21: //ADDU 
					EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.B;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x22: //SUB
					EX_MEM.ALUOutput = EX_MEM.A - EX_MEM.B;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x23: //SUBU
					EX_MEM.ALUOutput = EX_MEM.A - EX_MEM.B;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x24: //AND
					EX_MEM.ALUOutput = EX_MEM.A & EX_MEM.B;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x25: //OR
					EX_MEM.ALUOutput = EX_MEM.A | EX_MEM.B;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x26: //XOR
					EX_MEM.ALUOutput = EX_MEM.A ^ EX_MEM.B;
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x27: //NOR
					EX_MEM.ALUOutput = ~(EX_MEM.A | EX_MEM.B);
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x2A: //SLT
					if((EX_MEM.A & 0x80000000) == 0x80000000 && (EX_MEM.B & 0x80000000) == 0x80000000){  //Negative comparison
						if(EX_MEM.A < EX_MEM.B){
							//set
							EX_MEM.ALUOutput = 0x1;
						}else{
							//clear
							EX_MEM.ALUOutput = 0x0;
						}
					}else if((EX_MEM.A & 0x80000000) == 0 && (EX_MEM.B & 0x80000000) == 0){    //Positive compare
						if(EX_MEM.A < EX_MEM.B){
							//set
							EX_MEM.ALUOutput = 0x1;
						}else{
							//clear
							EX_MEM.ALUOutput = 0x0;
						}
					}else if((EX_MEM.A & 0x80000000) == 0 && (EX_MEM.B & 0x80000000) == 0x80000000){ //A positive, imm negative
						//clear
						EX_MEM.ALUOutput = 0x0;
					}else{ //A negative. imm positive
						//set
						EX_MEM.ALUOutput = 0x1;
					}
				
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					break;
				case 0x9: //JALR
					NEXT_STATE.PC = EX_MEM.A;
					EX_MEM.ALUOutput = CURRENT_STATE.PC + 4; //address of next instruction
					EX_MEM.destination = EX_MEM.registerRd;
					EX_MEM.register_register = 1;
					flush();
					break;
				case 0x8: //JR
					NEXT_STATE.PC = EX_MEM.A;
					flush();
					break;
				default:
					printf("Instruction at is not implemented!\n");
					break;
			}	
	}else if(EX_MEM.opcode == 0x1 && EX_MEM.IR != 0x00){
		//All special branches here... assuming the 'special' value for
		//other instructions is never 0x1
		
		//New switch for special branches
		switch(EX_MEM.registerRt){
			case 0x1: //BGEZ
				if((EX_MEM.A & 0x80000000) == 0){
					NEXT_STATE.PC = (CURRENT_STATE.PC + EX_MEM.imm) << 2;
					flush();
				}
				break;
			case 0x0: //BLTZ
				if((EX_MEM.A & 0x80000000) == 0x80000000){
					NEXT_STATE.PC = (CURRENT_STATE.PC + EX_MEM.imm) << 2;
					flush();
				}
				break;
			default:
					printf("\nCould not find the correct instruction! Special Branches");
				break;
		}
	}else if(EX_MEM.IR != 0x00){
		switch(EX_MEM.opcode){
			case 0x08: //ADDI
				EX_MEM.ALUOutput = EX_MEM.A + ( (EX_MEM.imm & 0x8000) > 0 ? (EX_MEM.imm | 0xFFFF0000) : (EX_MEM.imm & 0x0000FFFF));
				EX_MEM.destination = EX_MEM.registerRt;
				EX_MEM.register_immediate = 1;
				break;
			case 0x09: //ADDIU	instruction = IF_ID.IR ;
				EX_MEM.ALUOutput = EX_MEM.A + ( (EX_MEM.imm & 0x8000) > 0 ? (EX_MEM.imm | 0xFFFF0000) : (EX_MEM.imm & 0x0000FFFF));
				EX_MEM.destination = EX_MEM.registerRt;
				//printf("\nEX MEM DEST : %x", EX_MEM.destination);
				EX_MEM.register_immediate = 1;
				//printf("\nEX_MEM.Destination after set: %d", EX_MEM.destination);
				break;
			case 0x0A: //SLTI
				if((EX_MEM.A & 0x80000000) == 0x80000000 && (EX_MEM.imm & 0x80000000) == 0x80000000){  //Negative comparison
					if(EX_MEM.A < EX_MEM.imm){
						//set
						EX_MEM.ALUOutput = 0x1;
					}else{
						//clear
						EX_MEM.ALUOutput = 0x0;
					}
				}else if((EX_MEM.A & 0x80000000) == 0 && (EX_MEM.imm & 0x80000000) == 0){    //Positive compare
					if(EX_MEM.A < EX_MEM.imm){
						//set
						EX_MEM.ALUOutput = 0x1;
					}else{
						//clear
						EX_MEM.ALUOutput = 0x0;
					}
				}else if((EX_MEM.A & 0x80000000) == 0 && (EX_MEM.imm & 0x80000000) == 0x80000000){ //A positive, imm negative
					//clear
					EX_MEM.ALUOutput = 0x0;
				}else{ //A negative. imm positive
					//set
					EX_MEM.ALUOutput = 0x1;
				}
				
				EX_MEM.destination = EX_MEM.registerRt;
				EX_MEM.register_immediate = 1;
				break;
			case 0x0C: //ANDI
				EX_MEM.ALUOutput = EX_MEM.A & (EX_MEM.imm & 0x0000FFFF);
				EX_MEM.destination = EX_MEM.registerRt;
				EX_MEM.register_immediate = 1;
				break;
			case 0x0D: //ORI
				EX_MEM.ALUOutput = EX_MEM.A | (EX_MEM.imm & 0x0000FFFF);
				EX_MEM.destination = EX_MEM.registerRt;
				EX_MEM.register_immediate = 1;
				break;
			case 0x0E: //XORI
				//printf("\nXORI");
				//printf("\nEXMEM A : %x\n", EX_MEM.A);
				EX_MEM.ALUOutput = EX_MEM.A ^ (EX_MEM.imm & 0x0000FFFF);
				//printf("\nEXMEM ALU: %x", EX_MEM.ALUOutput);
				EX_MEM.destination = EX_MEM.registerRt;
				//printf("\nEXMEM dest: %x", EX_MEM.destination);
				EX_MEM.register_immediate = 1;
				break;
			case 0x0F: //LUI
				EX_MEM.ALUOutput = (EX_MEM.B & 0x0000FFFF) | (EX_MEM.imm << 16);
				EX_MEM.destination = EX_MEM.registerRt;
				EX_MEM.register_immediate = 1;
				break;
			case 0x20: //LB *******LOAD/STORE*********
        printf("\nLB");
			case 0x21: //LH *******LOAD/STORE*********
			case 0x23: //LW *******LOAD/STORE*********
				EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.imm;
				printf("\nEXMEM A: %x   EXMEM imm: %x   EXMEM ALUOUT: %x", EX_MEM.A, EX_MEM.imm, EX_MEM.ALUOutput);
				EX_MEM.destination = EX_MEM.registerRt;
				printf("\nEX_MEM DEST : %x", EX_MEM.destination);
				EX_MEM.memory_reference_load = 1;
				break;
			case 0x28: //SB *******LOAD/STORE*********
			case 0x29: //SH *******LOAD/STORE*********
			case 0x2B: //SW *******LOAD/STORE*********
        printf("\nEX_Store Word!");
				EX_MEM.ALUOutput = EX_MEM.A + EX_MEM.imm;
				EX_MEM.destination = 0;
				EX_MEM.memory_reference_store = 1;
				break;
			case 0x4: //BEQ
				if(EX_MEM.A == EX_MEM.B){
					NEXT_STATE.PC = CURRENT_STATE.PC + (EX_MEM.imm << 2);
					flush();
				}
				break;
			case 0x5: //BNE
				if(EX_MEM.A != EX_MEM.B){
					NEXT_STATE.PC = CURRENT_STATE.PC + (EX_MEM.imm << 2);
					flush();
				}
				break;
			case 0x6: //BLEZ
				if((EX_MEM.A & 0x80000000) == 0x80000000 || EX_MEM.A == 0){
					NEXT_STATE.PC = CURRENT_STATE.PC + (EX_MEM.imm << 2);
					flush();
				}
				break;
			case 0x7: //BGTZ
				if((EX_MEM.A & 0x80000000) != 0x80000000){
					NEXT_STATE.PC = CURRENT_STATE.PC + (EX_MEM.imm << 2);
					flush();
				}
				break;
			case 0x2:{ //J
				uint32_t target = (EX_MEM.IR & 0x3FFFFFF) << 2;
				uint32_t mask = CURRENT_STATE.PC & 0xF0000000;
				NEXT_STATE.PC = target | mask;
				flush();
				break;
			}
			case 0x3:{ //JAL
				uint32_t target = (EX_MEM.IR & 0x3FFFFFF) << 2;
				uint32_t mask = CURRENT_STATE.PC & 0xF0000000;
				EX_MEM.ALUOutput = CURRENT_STATE.PC + 4; //address of next instruction
				EX_MEM.destination = 31;
				EX_MEM.register_register = 1;
				NEXT_STATE.PC = EX_MEM.PC | target | 0x00;
				flush();
				break;
			}
			default:
				// put more things here
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	if(EX_MEM.register_immediate || EX_MEM.register_register || EX_MEM.memory_reference_load){
		EX_MEM.RegWrite = 1;
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	//Break IF_ID.IR into different parts ie. instruction and operands.
	uint32_t rs, rt, immediate, rd, opcode, function;
	
	opcode = (IF_ID.IR & 0xFC000000) >> 26;
	function = (IF_ID.IR & 0x0000003F);
	rs = (IF_ID.IR & 0x03E00000) >> 21;
	rt = (IF_ID.IR & 0x001F0000) >> 16;
	immediate = IF_ID.IR & 0x0000FFFF;
	rd = (IF_ID.IR & 0xF800) >> 11;
  
	

	//printf("regWrite: %d, destination: %d, forwarding: %d\n", EX_MEM.RegWrite, EX_MEM.destination, ENABLE_FORWARDING);
	if((EX_MEM.RegWrite && (EX_MEM.destination != 0) && (EX_MEM.destination == rs))){
		//Is forwarding enabled?
		if(ENABLE_FORWARDING){
			//Forward the data and dont stall.
			FORWARD_A = 10;
			//printf("\nFowarding A from EX_MEM");
		}else{
			stalling = 1;
		}
	}else if((EX_MEM.RegWrite && (EX_MEM.destination != 0) && (EX_MEM.destination == rt))){
		//Is forwarding enabled?
		if(ENABLE_FORWARDING){
			//Forward the data and dont stall.
			FORWARD_B = 10;
			//printf("\nFowarding B from EX_MEM");
		}else{
			stalling = 1;
		}
	}else if((MEM_WB.RegWrite && (MEM_WB.destination != 0) && (MEM_WB.destination == rs))){
		if(ENABLE_FORWARDING){
			//Forward the data from mem and dont stall.
			FORWARD_A = 01;
		}else{
			stalling = 1;
		}
	}else if(MEM_WB.RegWrite && (MEM_WB.destination != 0) && (MEM_WB.destination == rt)){
		if(ENABLE_FORWARDING){
			//Forward the data from mem and dont stall.
			FORWARD_B = 01;
		}else{
			stalling = 1;
		}
	}else if(EX_MEM.opcode == 0x00){
		switch(EX_MEM.function){
			case 0x9: //JALR
				stalling = 1;
				break;
			case 0x8: //JR
				stalling = 1;
				break;
			default:
				stalling = 0;
				break;
		}
	}else if(EX_MEM.opcode == 0x1){
		switch(EX_MEM.registerRt){
			case 0x1: //BGEZ
				stalling = 1;
				break;
			case 0x0: //BLTZ
				stalling = 1;
				break;
			default:
				stalling = 0;
				break;
		}
	}else{
		switch(EX_MEM.opcode){
			case 0x4: //BEQ
				stalling = 1;
				break;
			case 0x5: //BNE
				stalling = 1;
				break;
			case 0x6: //BLEZ
				stalling = 1;
				break;
			case 0x7: //BGTZ
				stalling = 1;
				break;
			case 0x2: //J
				stalling = 1;
				break;
			case 0x3: //JAL
				stalling = 1;
				break;
			default:
				stalling = 0;
				break;
		}
	}
  
  if(cacheStalling != 0){
    stalling = 1;
  }
  
	if(!stalling){
		ID_EX = IF_ID;
		memset(&IF_ID, 0, sizeof(IF_ID)); //Clear IF_ID
		ID_EX.registerRs = rs;
		ID_EX.registerRt = rt;
		ID_EX.registerRd = rd;
		ID_EX.opcode = opcode;
		ID_EX.function = function;
		//Data forwarding?
		if(!ENABLE_FORWARDING){
			ID_EX.A = NEXT_STATE.REGS[ID_EX.registerRs];
			ID_EX.B = NEXT_STATE.REGS[ID_EX.registerRt];
		}else{
			if(FORWARD_A == 01){
				//RS from MEM stage
				ID_EX.A = MEM_WB.LMD;
			}else if(FORWARD_A == 10){
				//RS from EX stage
				ID_EX.A = EX_MEM.ALUOutput;
			}
			
			if(FORWARD_B == 01){
				//RT from MEM stage
				ID_EX.B = MEM_WB.LMD;
			}else if(FORWARD_B == 10){
				//RT from EX stage
				ID_EX.B = EX_MEM.ALUOutput;
			}
			//Clear the flags
			FORWARD_A = 00;
			FORWARD_B = 00;
		}
		
		

		//Sign extension for immediate value
		if(ID_EX.IR & 0x00008000){
			//Negative
			uint32_t negative = 0xFFFF0000;
			ID_EX.imm = immediate | negative;
		}else{
			//Positive
			ID_EX.imm = immediate;
		}
	}
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	if(!stalling){
		IF_ID.IR = mem_read_32(CURRENT_STATE.PC);
		IF_ID.PC = CURRENT_STATE.PC;
		NEXT_STATE.PC += 4;
	}
	
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;
	
	instruction = mem_read_32(addr);
	
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	
	if(opcode == 0x00){
		/*R format instructions here*/
		
		switch(function){
			case 0x00:
				printf("SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				printf("SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				printf("SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				printf("JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					printf("JALR $r%u\n", rs);
				}
				else{
					printf("JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				printf("SYSCALL\n");
				break;
			case 0x10:
				printf("MFHI $r%u\n", rd);
				break;
			case 0x11:
				printf("MTHI $r%u\n", rs);
				break;
			case 0x12:
				printf("MFLO $r%u\n", rd);
				break;
			case 0x13:
				printf("MTLO $r%u\n", rs);
				break;
			case 0x18:
				printf("MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				printf("MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				printf("DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				printf("DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				printf("ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				printf("ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				printf("SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;//stall
			case 0x23:
				printf("SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				printf("AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				printf("OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				printf("XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				printf("NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				printf("SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					printf("BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					printf("BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				printf("J 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				printf("JAL 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				printf("BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				printf("BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				printf("BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				printf("BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				printf("ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				printf("ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				printf("SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				printf("ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				printf("ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				printf("XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				printf("LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				printf("LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				printf("LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				printf("LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				printf("SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				printf("SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				printf("SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
}
/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	printf("\nCurrent PC: %x", CURRENT_STATE.PC);
	printf("\nIF_ID.IR: %x", IF_ID.IR);
	printf("\nIF_ID.PC: %x", IF_ID.PC);
	printf("\nstalling: %d\n", stalling);
  printf("\nCache Stalling: %d", cacheStalling);
	
	printf("\nID_EX.IR: %x", ID_EX.IR);
	printf("\nID_EX.A: %x", ID_EX.A);
	printf("\nID_EX.B: %x", ID_EX.B);
	printf("\nID_EX.IMM: %x", ID_EX.imm);
	printf("\nID_EX.opcode: %x", ID_EX.opcode);
	printf("\nID_EX.function: %x", ID_EX.function);
	printf("\nstalling: %d\n", stalling);
  printf("\nCache Stalling: %d", cacheStalling);
	
	printf("\nEX_MEM.IR: %x", EX_MEM.IR);
	printf("\nEX_MEM.A: %x", EX_MEM.A);
	printf("\nEX_MEM.B: %x", EX_MEM.B);
	printf("\nEX_MEM.ALUOutput: %x", EX_MEM.ALUOutput);
	printf("\nstalling: %d\n", stalling);
  printf("\nCache Stalling: %d", cacheStalling);
	
	printf("\nMEM_WB.IR: %x", MEM_WB.IR);
	printf("\nMEM_WB.ALUOutput: %x", MEM_WB.ALUOutput);
	printf("\nMEM_WB.LMD: %x", MEM_WB.LMD);	
	printf("\nMEM_WB.memory_reference_load: %x", MEM_WB.memory_reference_load);	
	printf("\nMEM_WB.memory_reference_store: %x", MEM_WB.memory_reference_store);	
	printf("\nMEM_WB.register_register: %x", MEM_WB.register_register);	
	printf("\nMEM_WB.register_immediate: %x", MEM_WB.register_immediate);	
	printf("\nMEM_WB.MFHI: %d", MEM_WB.MFHI);	
	printf("\nMEM_WB.MTHI: %d", MEM_WB.MTHI);	
	printf("\nMEM_WB.MFLO: %d", MEM_WB.MFLO);	
	printf("\nMEM_WB.MTLO: %d", MEM_WB.MTLO);
	printf("\nMEM_WB.MULDIV: %d", MEM_WB.MULDIV);
	printf("\nstalling: %d\n", stalling);
  printf("\nCache Stalling: %d", cacheStalling);
	printf("\nENABLE_FORWARDING: %d", ENABLE_FORWARDING);
  
  printf("\nInstruction Count: %d", INSTRUCTION_COUNT);
	
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}

void flush(void){
	printf("flushing\n");
	memset(&IF_ID, 0, sizeof(EX_MEM));
	memset(&ID_EX, 0, sizeof(ID_EX));
}

void writeBufferToMemory(uint32_t blockAddress){
  mem_write_32(blockAddress, writeBuffer.words[0]);
  mem_write_32(blockAddress, writeBuffer.words[1]);
  mem_write_32(blockAddress, writeBuffer.words[2]);
  mem_write_32(blockAddress, writeBuffer.words[3]);
}

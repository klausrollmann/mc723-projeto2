/**
 * @file      mips_isa.cpp
 * @author    Sandro Rigo
 *            Marcus Bartholomeu
 *            Alexandro Baldassin (acasm information)
 *
 *            The ArchC Team
 *            http://www.archc.org/
 *
 *            Computer Systems Laboratory (LSC)
 *            IC-UNICAMP
 *            http://www.lsc.ic.unicamp.br/
 *
 * @version   1.0
 * @date      Mon, 19 Jun 2006 15:50:52 -0300
 * 
 * @brief     The ArchC i8051 functional model.
 * 
 * @attention Copyright (C) 2002-2006 --- The ArchC Team
 *
 */

#include  "mips_isa.H"
#include  "mips_isa_init.cpp"
#include  "mips_bhv_macros.H"

FILE * pFile;

/* Variaveis de configuração */
// Gerar traces
bool generate_traces = false;
// Tipo de branch predictor (um ou outro, ou nenhum)
bool branch_always_not_taken = false;
bool branch_predictor = false; // 1 bit predictor
// Com ou sem forwarding
bool forwarding = false;
// Superscalar
bool superscalar = false;
/* ************************ */

/* Contadores */
// Contador de Ciclos
long cycles = 0;
// Contador de Instruções
long intr = 0;

/* Hazards */
// Contador de stalls de dados
long data_stalls = 0;

int pipe_size = 5;

// Salva instruções anteriores na struct instruction
typedef struct instruction {
	int type; // tipo da instrução (0-aritmetica, 1-load)
	int w; // numero do registrador usado para escrita (-1 se nao for usado)
	int r1; // numero do registrador usado para leitura (-1 se nao for usado)
	int r2; // numero do registrador usado para leitura (-1 se nao for usado)
} instruction;

// Salva as duas instruções anteriores
instruction previous_instr[2]; // 1 = instrução anterior, 0 = instrução antes da anterior
instruction previous_instr2[2]; //1 = instrução anterior, 0 = instrução antes da anterior
instruction instr2; // instrução no mesmo estagio do pipeline

// Cria uma struct com os valores w, r1 e r2
instruction create_instr(int t, int w, int r1, int r2) {
	instruction new_instruction;
	new_instruction.type = t;
	new_instruction.w = w;
	new_instruction.r1 = r1;
	new_instruction.r2 = r2;
	return new_instruction;
}

// Atualiza possíveis hazards
// TODO: Tem que adicionar possíveis hazards de processador com varios estagios
void update_data_hazard(instruction instr) {
	// Se nao for superescalarar
	if (!superscalar) {
		// Se nao tiver forwarding verifica os stalls necessarios
		if (!forwarding) {
			// Verifica se le em r1 algum registrador que foi escrito por uma das previous_instr
			if (instr.r1 > 0) { 
				if (instr.r1 == previous_instr[1].w) {// Dois stalls
					data_stalls += 2;
					cycles += 2;
                                        if (pipe_size == 7){
                                            data_stalls+=2;
                                            cycles +=2;
                                        }
				}
				else if (instr.r1 == previous_instr[0].w) { // Um stall
					data_stalls += 1;
					cycles += 1;
                                        if (pipe_size == 7){
                                            data_stalls+=1;
                                            cycles +=1;
                                        }
				}
				
			}
			// Verifica se le em rt algum registrador que foi escrito por uma das previous_instr
			else if (instr.r2 > 0) { 
				if (instr.r2 == previous_instr[1].w) {// Dois stalls
					data_stalls += 2;
					cycles += 2;
				}
				else if (instr.r2 == previous_instr[0].w) { // Um stall
					data_stalls += 1;
					cycles += 1;
				}
			}
		}
		// Com Forwarding
		else {
			if (previous_instr[1].type == 1) { // Load
				if (instr.r1 == previous_instr[1].w) {
					data_stalls += 1;
					cycles += 1;
				}
				else if (instr.r2 == previous_instr[1].w) {
					data_stalls += 1;
					cycles += 1;
				}
			}
		}
		// Atualiza previous_instr
		previous_instr[0] = previous_instr[1];
		previous_instr[1] = instr;
	}
	// Processador superescalar (com forwarding)
	else {
		if (intr % 2 == 1) {
			instr2 = instr;
		}
		else {
		// RAW
		if (previous_instr[1].type == 1) {
			if (instr.r1 == previous_instr[1].w || instr2.r1 == previous_instr[1].w) {
				data_stalls += 1;
				cycles += 1;
			}
			else if (instr.r2 == previous_instr[1].w || instr2.r2 == previous_instr[1].w) {
				data_stalls += 1;
				cycles += 1;
			}
		}
		if (previous_instr2[1].type == 1) {
			if (instr.r1 == previous_instr2[1].w || instr2.r1 == previous_instr2[1].w) {
				data_stalls += 1;
				cycles += 1;
			}
			else if (instr.r2 == previous_instr2[1].w || instr2.r2 == previous_instr2[1].w) {
				data_stalls += 1;
				cycles += 1;
			}
		}
		// WAR
		if ((instr.r1 == instr2.w) || (instr.r2 = instr2.w)  || (instr2.r1 == instr.w) || (instr2.r2 = instr.w)) {
			data_stalls += 1;
			cycles += 1;
		}
		// WAW 
		else if (instr.w == instr2.w) {
			data_stalls += 1;
			cycles += 1;
		}
		// Atualiza previous_instr
		previous_instr[0] = previous_instr[1];
		previous_instr[1] = instr;
		previous_instr2[0] = previous_instr2[1];
		previous_instr2[1] = instr2;
		}

	}
}


/* Branch predictor */
// Contador de branches corretos e incorretos
long branch_correct = 0;
long branch_incorrect = 0;
// Contador de stalls devido a branches
long branch_stalls = 0;
long jump_stalls = 0;

// Marcador do ultimo predict
bool last_branch = false; // false se nao for taken e true se for taken

// Atualiza ciclos se der jump
void jump_taken() {
	jump_stalls += 2;	
	cycles += 2;	
}
// Atualiza se for taken
void branch_taken() {
	// Atualiza de acordo com o branch predictor usado
	if (branch_predictor) { // com branch predictor 1-bit
		if (last_branch == true) branch_correct++; // se acertar
		else { // se errar atualiza as variaveis
			branch_incorrect++;
			branch_stalls += 3;
			cycles += 3;
		}
	}
	else if (branch_always_not_taken) { // com branch always not taken
		branch_incorrect++; // foi taken, entao errou
		branch_stalls += 3;
		cycles += 3;
	}
	else { // sem branch predictor
		branch_incorrect++; // sempre erra
		branch_stalls += 3;
		cycles += 3;
	}
	last_branch = true; // atualiza ultimo branch como taken
}
// Atualiza se não for taken
void branch_not_taken() {
	// Atualiza de acordo com o branch predictor usado
	if (branch_predictor) { // com branch predictor 1-bit
		if (last_branch == false) branch_correct++; // se acertar
		else { // se errar atualiza as variaveis
			branch_incorrect++;
			branch_stalls += 3;
			cycles += 3;
		}
	}
	else if (branch_always_not_taken) { // com branch always not taken
		branch_correct++; // nao foi taken, entao acertou
	}
	else { // sem branch predictor
		branch_incorrect++; // sempre erra
		branch_stalls += 3;
		cycles += 3;
	}
	last_branch = false; // atualiza ultimo branch como not taken
}



//If you want debug information for this model, uncomment next line
//#define DEBUG_MODEL
#include "ac_debug_model.H"


//!User defined macros to reference registers.
#define Ra 31
#define Sp 29

// 'using namespace' statement to allow access to all
// mips-specific datatypes
using namespace mips_parms;

static int processors_started = 0;
#define DEFAULT_STACK_SIZE (256*1024)

//!Generic instruction behavior method.
void ac_behavior( instruction )
{ 
	intr++;
	cycles++;
	dbg_printf("----- PC=%#x ----- %lld\n", (int) ac_pc, ac_instr_counter);
	//  dbg_printf("----- PC=%#x NPC=%#x ----- %lld\n", (int) ac_pc, (int)npc, ac_instr_counter);
#ifndef NO_NEED_PC_UPDATE
	ac_pc = npc;
	npc = ac_pc + 4;
# endif 
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 2, (int)ac_pc);
};

//! Instruction Format behavior methods.
void ac_behavior( Type_R ){}
void ac_behavior( Type_I ){}
void ac_behavior( Type_J ){}

//!Behavior called before starting simulation
void ac_behavior(begin)
{
	cycles = 0;
	intr = 0;
	dbg_printf("@@@ begin behavior @@@\n");
	RB[0] = 0;
	npc = ac_pc + 4;

	// It is not required by the architecture, but makes debug really easier
	for (int regNum = 0; regNum < 32; regNum ++)
		RB[regNum] = 0;
	hi = 0;
	lo = 0;

	RB[29] =  AC_RAM_END - 1024 - processors_started++ * DEFAULT_STACK_SIZE;
	//opens the file to write the trace
	if (generate_traces) {
		printf("INFO: Abriu arquivo de traces.\n");
		pFile = fopen ("/home/staff/lucas/tmp-lab/g05-trace_susan_corners.din","w");
	}
}

//!Behavior called after finishing simulation
void ac_behavior(end)
{
	// Imprime configurações
	printf("---- Configurações ---- \n\n");
	printf("Generate Traces = ");
	(generate_traces) ? printf("true\n"):printf("false\n");
	printf("Forwarding = ");
	(forwarding) ? printf("true\n"):printf("false\n");
	printf("Branch predictor = ");
	if (branch_predictor) printf("1-Bit Taken\n");
	else if (branch_always_not_taken) printf("Always not taken\n");
	else printf("None\n");
	printf("Type = ");
	(superscalar) ? printf("superescalar\n"):printf("escalar\n");

	// Imprime informações
	if (superscalar) {
		cycles /= 2;
	}
	printf("Cycles: %ld\n", cycles);
	printf("Total Instructions: %ld\n", intr);
	printf("Total Stalled Instructions: %ld\n", data_stalls + branch_stalls + jump_stalls);
	printf("|--Data Stalls: %ld\n", data_stalls);
	printf("|--Branch Stalls: %ld\n", branch_stalls);
	printf("|--Jump Stalls: %ld\n", jump_stalls);
	printf("Correct Branch Predictions: %ld\n", branch_correct);
	printf("Total Branches: %ld\n", branch_correct + branch_incorrect);
	printf("CPI: %.2f\n", (float)cycles/intr);
	dbg_printf("@@@ end behavior @@@\n");
	if (generate_traces) {
		fclose(pFile);
		printf("INFO: Fechou arquivo de traces.\n");
	}
}


/**** LOADS ****/
//!Instruction lb behavior method.
void ac_behavior( lb )
{
	char byte;
	dbg_printf("lb r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	byte = DM.read_byte(RB[rs]+ imm);
	RB[rt] = (ac_Sword)byte ;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 0, (RB[rs] + imm));
};

//!Instruction lbu behavior method.
void ac_behavior( lbu )
{
	unsigned char byte;
	dbg_printf("lbu r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	byte = DM.read_byte(RB[rs]+ imm);
	RB[rt] = byte ;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 0, (RB[rs] + imm));
};

//!Instruction lh behavior method.
void ac_behavior( lh )
{
	short int half;
	dbg_printf("lh r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	half = DM.read_half(RB[rs]+ imm);
	RB[rt] = (ac_Sword)half ;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 0, (RB[rs] + imm));
};

//!Instruction lhu behavior method.
void ac_behavior( lhu )
{
	unsigned short int  half;
	half = DM.read_half(RB[rs]+ imm);
	RB[rt] = half ;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 0, (RB[rs] + imm));
};

//!Instruction lw behavior method.
void ac_behavior( lw )
{
	dbg_printf("lw r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	RB[rt] = DM.read(RB[rs]+ imm);
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 0, (RB[rs] + imm));
};

//!Instruction lwl behavior method.
void ac_behavior( lwl )
{
	dbg_printf("lwl r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	unsigned int addr, offset;
	ac_Uword data;

	addr = RB[rs] + imm;
	offset = (addr & 0x3) * 8;
	data = DM.read(addr & 0xFFFFFFFC);
	data <<= offset;
	data |= RB[rt] & ((1<<offset)-1);
	RB[rt] = data;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 0, addr & 0xFFFFFFFC);
};

//!Instruction lwr behavior method.
void ac_behavior( lwr )
{
	dbg_printf("lwr r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	unsigned int addr, offset;
	ac_Uword data;

	addr = RB[rs] + imm;
	offset = (3 - (addr & 0x3)) * 8;
	data = DM.read(addr & 0xFFFFFFFC);
	data >>= offset;
	data |= RB[rt] & (0xFFFFFFFF << (32-offset));
	RB[rt] = data;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 0, addr & 0xFFFFFFFC);
};

/**** STORES ****/
//!Instruction sb behavior method.
void ac_behavior( sb )
{
	unsigned char byte;
	dbg_printf("sb r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	byte = RB[rt] & 0xFF;
	DM.write_byte(RB[rs] + imm, byte);
	dbg_printf("Result = %#x\n", (int) byte);
	update_data_hazard(create_instr(0, -1, rs, rt));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 1, RB[rs] + imm);
};

//!Instruction sh behavior method.
void ac_behavior( sh )
{
	unsigned short int half;
	dbg_printf("sh r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	half = RB[rt] & 0xFFFF;
	DM.write_half(RB[rs] + imm, half);
	dbg_printf("Result = %#x\n", (int) half);
	update_data_hazard(create_instr(0, -1, rs, rt));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 1, RB[rs] + imm);
};

//!Instruction sw behavior method.
void ac_behavior( sw )
{
	dbg_printf("sw r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	DM.write(RB[rs] + imm, RB[rt]);
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(0, -1, rs, rt));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 1, RB[rs] + imm);
};

//!Instruction swl behavior method.
void ac_behavior( swl )
{
	dbg_printf("swl r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	unsigned int addr, offset;
	ac_Uword data;

	addr = RB[rs] + imm;
	offset = (addr & 0x3) * 8;
	data = RB[rt];
	data >>= offset;
	data |= DM.read(addr & 0xFFFFFFFC) & (0xFFFFFFFF << (32-offset));
	DM.write(addr & 0xFFFFFFFC, data);
	dbg_printf("Result = %#x\n", data);
	update_data_hazard(create_instr(0, -1, rs, rt));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 1, addr & 0xFFFFFFFC);
};

//!Instruction swr behavior method.
void ac_behavior( swr )
{

	dbg_printf("swr r%d, %d(r%d)\n", rt, imm & 0xFFFF, rs);
	unsigned int addr, offset;
	ac_Uword data;

	addr = RB[rs] + imm;
	offset = (3 - (addr & 0x3)) * 8;
	data = RB[rt];
	data <<= offset;
	data |= DM.read(addr & 0xFFFFFFFC) & ((1<<offset)-1);
	DM.write(addr & 0xFFFFFFFC, data);
	dbg_printf("Result = %#x\n", data);
	update_data_hazard(create_instr(0, -1, rs, rt));
	if (generate_traces)
		fprintf(pFile, "%d %x\n", 1, addr & 0xFFFFFFFC);
};

/**** ADDI ****/
//!Instruction addi behavior method.
void ac_behavior( addi )
{
	dbg_printf("addi r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	RB[rt] = RB[rs] + imm;
	dbg_printf("Result = %#x\n", RB[rt]);
	//Test overflow
	if ( ((RB[rs] & 0x80000000) == (imm & 0x80000000)) &&
			((imm & 0x80000000) != (RB[rt] & 0x80000000)) ) {
		fprintf(stderr, "EXCEPTION(addi): integer overflow.\n"); exit(EXIT_FAILURE);
	}
	update_data_hazard(create_instr(0, rt, rs, -1));
};

//!Instruction addiu behavior method.
void ac_behavior( addiu )
{
	dbg_printf("addiu r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	RB[rt] = RB[rs] + imm;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(0, rt, rs, -1));
};

/**** BIT OPERATIONS ****/
//!Instruction slti behavior method.
void ac_behavior( slti )
{
	dbg_printf("slti r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	// Set the RD if RS< IMM
	if( (ac_Sword) RB[rs] < (ac_Sword) imm )
		RB[rt] = 1;
	// Else reset RD
	else
		RB[rt] = 0;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(0, rt, rs, -1));
};

//!Instruction sltiu behavior method.
void ac_behavior( sltiu )
{
	dbg_printf("sltiu r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	// Set the RD if RS< IMM
	if( (ac_Uword) RB[rs] < (ac_Uword) imm )
		RB[rt] = 1;
	// Else reset RD
	else
		RB[rt] = 0;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(0, rt, rs, -1));
};

//!Instruction andi behavior method.
void ac_behavior( andi )
{	
	dbg_printf("andi r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	RB[rt] = RB[rs] & (imm & 0xFFFF) ;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(0, rt, rs, -1));
};

//!Instruction ori behavior method.
void ac_behavior( ori )
{	
	dbg_printf("ori r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	RB[rt] = RB[rs] | (imm & 0xFFFF) ;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(0, rt, rs, -1));
};

//!Instruction xori behavior method.
void ac_behavior( xori )
{	
	dbg_printf("xori r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	RB[rt] = RB[rs] ^ (imm & 0xFFFF) ;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(0, rt, rs, -1));
};

//!Instruction lui behavior method.
void ac_behavior( lui )
{	
	dbg_printf("lui r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	// Load a constant in the upper 16 bits of a register
	// To achieve the desired behaviour, the constant was shifted 16 bits left
	// and moved to the target register ( rt )
	RB[rt] = imm << 16;
	dbg_printf("Result = %#x\n", RB[rt]);
	update_data_hazard(create_instr(1, rt, rs, -1));
};

/**** ARITHMETIC ****/
//!Instruction add behavior method.
void ac_behavior( add )
{
	dbg_printf("add r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = RB[rs] + RB[rt];
	dbg_printf("Result = %#x\n", RB[rd]);
	//Test overflow
	if ( ((RB[rs] & 0x80000000) == (RB[rd] & 0x80000000)) &&
			((RB[rd] & 0x80000000) != (RB[rt] & 0x80000000)) ) {
		fprintf(stderr, "EXCEPTION(add): integer overflow.\n"); exit(EXIT_FAILURE);
	}
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction addu behavior method.
void ac_behavior( addu )
{
	dbg_printf("addu r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = RB[rs] + RB[rt];
	//cout << "  RS: " << (unsigned int)RB[rs] << " RT: " << (unsigned int)RB[rt] << endl;
	//cout << "  Result =  " <<  (unsigned int)RB[rd] <<endl;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction sub behavior method.
void ac_behavior( sub )
{
	dbg_printf("sub r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = RB[rs] - RB[rt];
	dbg_printf("Result = %#x\n", RB[rd]);
	//TODO: test integer overflow exception for sub
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction subu behavior method.
void ac_behavior( subu )
{
	dbg_printf("subu r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = RB[rs] - RB[rt];
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

/**** SETS ****/
//!Instruction slt behavior method.
void ac_behavior( slt )
{	
	dbg_printf("slt r%d, r%d, r%d\n", rd, rs, rt);
	// Set the RD if RS< RT
	if( (ac_Sword) RB[rs] < (ac_Sword) RB[rt] )
		RB[rd] = 1;
	// Else reset RD
	else
		RB[rd] = 0;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction sltu behavior method.
void ac_behavior( sltu )
{
	dbg_printf("sltu r%d, r%d, r%d\n", rd, rs, rt);
	// Set the RD if RS < RT
	if( RB[rs] < RB[rt] )
		RB[rd] = 1;
	// Else reset RD
	else
		RB[rd] = 0;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction instr_and behavior method.
void ac_behavior( instr_and )
{
	dbg_printf("instr_and r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = RB[rs] & RB[rt];
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction instr_or behavior method.
void ac_behavior( instr_or )
{
	dbg_printf("instr_or r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = RB[rs] | RB[rt];
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction instr_xor behavior method.
void ac_behavior( instr_xor )
{
	dbg_printf("instr_xor r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = RB[rs] ^ RB[rt];
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction instr_nor behavior method.
void ac_behavior( instr_nor )
{
	dbg_printf("nor r%d, r%d, r%d\n", rd, rs, rt);
	RB[rd] = ~(RB[rs] | RB[rt]);
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction nop behavior method.
void ac_behavior( nop )
{  
	dbg_printf("nop\n");
	update_data_hazard(create_instr(0, -1, -1, -1));
};

/**** SHIFTS ****/
//!Instruction sll behavior method.
void ac_behavior( sll )
{  
	dbg_printf("sll r%d, r%d, %d\n", rd, rs, shamt);
	RB[rd] = RB[rt] << shamt;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, -1));
};

//!Instruction srl behavior method.
void ac_behavior( srl )
{
	dbg_printf("srl r%d, r%d, %d\n", rd, rs, shamt);
	RB[rd] = RB[rt] >> shamt;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, -1));
};

//!Instruction sra behavior method.
void ac_behavior( sra )
{
	dbg_printf("sra r%d, r%d, %d\n", rd, rs, shamt);
	RB[rd] = (ac_Sword) RB[rt] >> shamt;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, -1));
};

//!Instruction sllv behavior method.
void ac_behavior( sllv )
{
	dbg_printf("sllv r%d, r%d, r%d\n", rd, rt, rs);
	RB[rd] = RB[rt] << (RB[rs] & 0x1F);
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction srlv behavior method.
void ac_behavior( srlv )
{
	dbg_printf("srlv r%d, r%d, r%d\n", rd, rt, rs);
	RB[rd] = RB[rt] >> (RB[rs] & 0x1F);
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

//!Instruction srav behavior method.
void ac_behavior( srav )
{
	dbg_printf("srav r%d, r%d, r%d\n", rd, rt, rs);
	RB[rd] = (ac_Sword) RB[rt] >> (RB[rs] & 0x1F);
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, rt, rs));
};

/**** MULT/DIV ****/
//!Instruction mult behavior method.
void ac_behavior( mult )
{
	dbg_printf("mult r%d, r%d\n", rs, rt);

	long long result;
	int half_result;

	result = (ac_Sword) RB[rs];
	result *= (ac_Sword) RB[rt];

	half_result = (result & 0xFFFFFFFF);
	// Register LO receives 32 less significant bits
	lo = half_result;

	half_result = ((result >> 32) & 0xFFFFFFFF);
	// Register HI receives 32 most significant bits
	hi = half_result ;

	dbg_printf("Result = %#llx\n", result);
	update_data_hazard(create_instr(0, -1, rt, rs));
};

//!Instruction multu behavior method.
void ac_behavior( multu )
{
	dbg_printf("multu r%d, r%d\n", rs, rt);

	unsigned long long result;
	unsigned int half_result;

	result  = RB[rs];
	result *= RB[rt];

	half_result = (result & 0xFFFFFFFF);
	// Register LO receives 32 less significant bits
	lo = half_result;

	half_result = ((result>>32) & 0xFFFFFFFF);
	// Register HI receives 32 most significant bits
	hi = half_result ;

	dbg_printf("Result = %#llx\n", result);
	update_data_hazard(create_instr(0, -1, rt, rs));
};

//!Instruction div behavior method.
void ac_behavior( div )
{
	dbg_printf("div r%d, r%d\n", rs, rt);
	// Register LO receives quotient
	lo = (ac_Sword) RB[rs] / (ac_Sword) RB[rt];
	// Register HI receives remainder
	hi = (ac_Sword) RB[rs] % (ac_Sword) RB[rt];
	update_data_hazard(create_instr(0, -1, rt, rs));
};

//!Instruction divu behavior method.
void ac_behavior( divu )
{
	dbg_printf("divu r%d, r%d\n", rs, rt);
	// Register LO receives quotient
	lo = RB[rs] / RB[rt];
	// Register HI receives remainder
	hi = RB[rs] % RB[rt];
	update_data_hazard(create_instr(0, -1, rt, rs));
};

//!Instruction mfhi behavior method.
void ac_behavior( mfhi )
{
	dbg_printf("mfhi r%d\n", rd);
	RB[rd] = hi;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, -1, -1));
};

//!Instruction mthi behavior method.
void ac_behavior( mthi )
{
	dbg_printf("mthi r%d\n", rs);
	hi = RB[rs];
	dbg_printf("Result = %#x\n", (unsigned int) hi);
	update_data_hazard(create_instr(0, -1, rs, -1));
};

//!Instruction mflo behavior method.
void ac_behavior( mflo )
{
	dbg_printf("mflo r%d\n", rd);
	RB[rd] = lo;
	dbg_printf("Result = %#x\n", RB[rd]);
	update_data_hazard(create_instr(0, rd, -1, -1));
};

//!Instruction mtlo behavior method.
void ac_behavior( mtlo )
{
	dbg_printf("mtlo r%d\n", rs);
	lo = RB[rs];
	dbg_printf("Result = %#x\n", (unsigned int) lo);
	update_data_hazard(create_instr(0, -1, rs, -1));
};

/**** JUMPS ****/
//!Instruction j behavior method.
void ac_behavior( j )
{
	jump_taken();		
	dbg_printf("j %d\n", addr);
	addr = addr << 2;
#ifndef NO_NEED_PC_UPDATE
	npc =  (ac_pc & 0xF0000000) | addr;
#endif 
	dbg_printf("Target = %#x\n", (ac_pc & 0xF0000000) | addr );
	update_data_hazard(create_instr(0, -1, -1, -1));
};

//!Instruction jal behavior method.
void ac_behavior( jal )
{
	jump_taken();		
	dbg_printf("jal %d\n", addr);
	// Save the value of PC + 8 (return address) in $ra ($31) and
	// jump to the address given by PC(31...28)||(addr<<2)
	// It must also flush the instructions that were loaded into the pipeline
	RB[Ra] = ac_pc+4; //ac_pc is pc+4, we need pc+8

	addr = addr << 2;
#ifndef NO_NEED_PC_UPDATE
	npc = (ac_pc & 0xF0000000) | addr;
#endif 

	dbg_printf("Target = %#x\n", (ac_pc & 0xF0000000) | addr );
	dbg_printf("Return = %#x\n", ac_pc+4);
	update_data_hazard(create_instr(0, Ra, -1, -1));
};

//!Instruction jr behavior method.
void ac_behavior( jr )
{
	jump_taken();		
	dbg_printf("jr r%d\n", rs);
	// Jump to the address stored on the register reg[RS]
	// It must also flush the instructions that were loaded into the pipeline
#ifndef NO_NEED_PC_UPDATE
	npc = RB[rs], 1;
#endif 
	dbg_printf("Target = %#x\n", RB[rs]);
	update_data_hazard(create_instr(0, -1, rs, -1));
};

//!Instruction jalr behavior method.
void ac_behavior( jalr )
{
	jump_taken();		
	dbg_printf("jalr r%d, r%d\n", rd, rs);
	// Save the value of PC + 8(return address) in rd and
	// jump to the address given by [rs]

#ifndef NO_NEED_PC_UPDATE
	npc = RB[rs], 1;
#endif 
	dbg_printf("Target = %#x\n", RB[rs]);

	if( rd == 0 )  //If rd is not defined use default
		rd = Ra;
	RB[rd] = ac_pc+4;
	dbg_printf("Return = %#x\n", ac_pc+4);
	update_data_hazard(create_instr(0, rd, rs, -1));
};

/**** BRANCHES ****/
//!Instruction beq behavior method.
void ac_behavior( beq )
{
	bool taken = false;
	dbg_printf("beq r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	if( RB[rs] == RB[rt] ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2);
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	update_data_hazard(create_instr(0, -1, rs, rt));

};

//!Instruction bne behavior method.
void ac_behavior( bne )
{	
	bool taken = false;
	dbg_printf("bne r%d, r%d, %d\n", rt, rs, imm & 0xFFFF);
	if( RB[rs] != RB[rt] ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2);
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	update_data_hazard(create_instr(0, -1, rs, rt));
};

//!Instruction blez behavior method.
void ac_behavior( blez )
{
	bool taken = false;
	dbg_printf("blez r%d, %d\n", rs, imm & 0xFFFF);
	if( (RB[rs] == 0 ) || (RB[rs]&0x80000000 ) ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2), 1;
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	update_data_hazard(create_instr(0, -1, rs, -1));
};

//!Instruction bgtz behavior method.
void ac_behavior( bgtz )
{
	bool taken = false;
	dbg_printf("bgtz r%d, %d\n", rs, imm & 0xFFFF);
	if( !(RB[rs] & 0x80000000) && (RB[rs]!=0) ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2);
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	update_data_hazard(create_instr(0, -1, rs, -1));
};

//!Instruction bltz behavior method.
void ac_behavior( bltz )
{
	bool taken = false;
	dbg_printf("bltz r%d, %d\n", rs, imm & 0xFFFF);
	if( RB[rs] & 0x80000000 ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2);
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	update_data_hazard(create_instr(0, -1, rs, -1));
};

//!Instruction bgez behavior method.
void ac_behavior( bgez )
{
	bool taken = false;
	dbg_printf("bgez r%d, %d\n", rs, imm & 0xFFFF);
	if( !(RB[rs] & 0x80000000) ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2);
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	update_data_hazard(create_instr(0, -1, rs, -1));
};

//!Instruction bltzal behavior method.
void ac_behavior( bltzal )
{
	bool taken = false;
	dbg_printf("bltzal r%d, %d\n", rs, imm & 0xFFFF);
	RB[Ra] = ac_pc+4; //ac_pc is pc+4, we need pc+8
	if( RB[rs] & 0x80000000 ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2);
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	dbg_printf("Return = %#x\n", ac_pc+4);
	update_data_hazard(create_instr(0, Ra, rs, -1));
};

//!Instruction bgezal behavior method.
void ac_behavior( bgezal )
{
	bool taken = false;
	dbg_printf("bgezal r%d, %d\n", rs, imm & 0xFFFF);
	RB[Ra] = ac_pc+4; //ac_pc is pc+4, we need pc+8
	if( !(RB[rs] & 0x80000000) ){
#ifndef NO_NEED_PC_UPDATE
		npc = ac_pc + (imm<<2);
#endif 
		taken = true;
		dbg_printf("Taken to %#x\n", ac_pc + (imm<<2));
	}	
	if (taken) branch_taken();
	else branch_not_taken();		
	dbg_printf("Return = %#x\n", ac_pc+4);
	update_data_hazard(create_instr(0, Ra, rs, -1));
};

/**** OTHER ****/
//!Instruction sys_call behavior method.
void ac_behavior( sys_call )
{
	dbg_printf("syscall\n");
	stop();
	update_data_hazard(create_instr(0, -1, -1, -1));
}

//!Instruction instr_break behavior method.
void ac_behavior( instr_break )
{
	fprintf(stderr, "instr_break behavior not implemented.\n"); 
	exit(EXIT_FAILURE);
	update_data_hazard(create_instr(0, -1, -1, -1));
}

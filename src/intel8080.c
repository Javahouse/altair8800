#include "intel8080.h"
#include "op_codes.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
	#include <Windows.h>
#endif

uint8_t get_parity(uint8_t val)
{
	uint8_t parity = 1;
	while(val)
	{
		parity = !parity;
		val = val & (val - 1);
	}

	return parity;
}

void i8080_reset(intel8080_t *cpu, port_in in, port_out out, disk_controller_t *disk_controller)
{
	memset(cpu, 0, sizeof(intel8080_t));
	cpu->term_in = in;
	cpu->term_out = out;
	cpu->disk_controller = *disk_controller;
	cpu->registers.flags = 0x2;
}

int i8080_check_carry(uint16_t a, uint16_t b)
{
	if((a + b) > 0xff)
		return 1;
	else
		return 0;
}

int i8080_check_half_carry(uint16_t a, uint16_t b)
{
	a &= 0xf;
	b &= 0xf;

	if((a + b) > 0xf)
		return 1;
	else
		return 0;
}


void i8080_mwrite(intel8080_t *cpu)
{
	{
		cpu->memory[cpu->address_bus] = cpu->data_bus;
	}
}

void i8080_mread(intel8080_t *cpu)
{
	{
		cpu->data_bus = cpu->memory[cpu->address_bus];
	}
}

void i8080_pairwrite(intel8080_t *cpu, uint8_t pair, uint16_t val)
{
	switch(pair)
	{
	case PAIR_BC:
		cpu->registers.bc = val;
		break;
	case PAIR_DE:
		cpu->registers.de = val;
		break;
	case PAIR_HL:
		cpu->registers.hl = val;
		break;
	case PAIR_SP:
		cpu->registers.sp = val;
		break;
	}
}

uint16_t i8080_pairread(intel8080_t *cpu, uint8_t pair)
{
	switch(pair)
	{
	case PAIR_BC:
		return cpu->registers.bc; 
		break;
	case PAIR_DE:
		return cpu->registers.de;
		break;
	case PAIR_HL:
		return cpu->registers.hl;
		break;
	case PAIR_SP:
		return cpu->registers.sp;
		break;
	default:
		return 0;
	}
}

void i8080_regwrite(intel8080_t *cpu, uint8_t reg, uint8_t val)
{
	switch(reg)
	{
	case REGISTER_A:
		cpu->registers.a = val;
		break;
	case REGISTER_B:
		cpu->registers.b = val;
		break;
	case REGISTER_C:
		cpu->registers.c = val;
		break;
	case REGISTER_D:
		cpu->registers.d = val;
		break;
	case REGISTER_E:
		cpu->registers.e = val;
		break;
	case REGISTER_H:
		cpu->registers.h = val;
		break;
	case REGISTER_L:
		cpu->registers.l = val;
		break;
	case MEMORY_ACCESS:
		cpu->address_bus = cpu->registers.hl;
		cpu->data_bus = val;
		i8080_mwrite(cpu);
	}
}

uint8_t i8080_regread(intel8080_t *cpu, uint8_t reg)
{
	switch(reg)
	{
	case REGISTER_A:
		return cpu->registers.a;
		break;
	case REGISTER_B:
		return cpu->registers.b;
		break;
	case REGISTER_C:
		return cpu->registers.c;
		break;
	case REGISTER_D:
		return cpu->registers.d;
		break;
	case REGISTER_E:
		return cpu->registers.e;
		break;
	case REGISTER_H:
		return cpu->registers.h;
		break;
	case REGISTER_L:
		return cpu->registers.l;
		break;
	case MEMORY_ACCESS:
		cpu->address_bus = cpu->registers.hl;
		i8080_mread(cpu);
		return cpu->data_bus;
	default:
		return 0;
	}
}

uint8_t i8080_check_condition(intel8080_t *cpu, uint8_t condition)
{
	switch(condition)
	{
	case CONDITION_NZ:
		return !(cpu->registers.flags & FLAGS_ZERO);
	case CONDITION_Z:
		return (cpu->registers.flags & FLAGS_ZERO);
	case CONDITION_NC:
		return !(cpu->registers.flags & FLAGS_CARRY);
	case CONDITION_C:
		return (cpu->registers.flags & FLAGS_CARRY);
	case CONDITION_PO:
		return !(cpu->registers.flags & FLAGS_PARITY);
	case CONDITION_PE:
		return (cpu->registers.flags & FLAGS_PARITY);
	case CONDITION_P:
		return !(cpu->registers.flags & FLAGS_SIGN);
	case CONDITION_M:
		return (cpu->registers.flags & FLAGS_SIGN);
	}
	return 0;
}

void i8080_examine(intel8080_t *cpu, uint16_t address)
{
	cpu->data_bus = OP_JMP;
	i8080_cycle(cpu);
	cpu->data_bus = address & 0xff;
	i8080_cycle(cpu);
	cpu->data_bus = (address >> 8) & 0xff;
	i8080_cycle(cpu);

	i8080_sync(cpu);
}

void i8080_examine_next(intel8080_t *cpu)
{
	cpu->data_bus = OP_NOP;
	i8080_cycle(cpu);
	i8080_sync(cpu);
}

void i8080_deposit(intel8080_t *cpu, uint8_t data)
{
	cpu->data_bus = data;
	i8080_mwrite(cpu);
}

void i8080_deposit_next(intel8080_t *cpu, uint8_t data)
{
	i8080_examine_next(cpu);
	cpu->data_bus = data;
	i8080_mwrite(cpu);
}

void i8080_set_flag(intel8080_t *cpu, uint8_t flag)
{
	cpu->registers.flags |= flag;
}

void i8080_clear_flag(intel8080_t *cpu, uint8_t flag)
{
	cpu->registers.flags &= ~flag;
}

void i8080_update_flags(intel8080_t *cpu, uint8_t reg, uint8_t mask)
{
	uint8_t val = i8080_regread(cpu, reg);
	if(mask & FLAGS_PARITY)
	{
		if(get_parity(val))
			i8080_set_flag(cpu, FLAGS_PARITY);
		else
			i8080_clear_flag(cpu, FLAGS_PARITY);
			
	}
	if(mask & FLAGS_H)
	{
		
	}
	if(mask & FLAGS_IF)
	{

	}
	if(mask & FLAGS_ZERO)
	{	
		if(val == 0)
			i8080_set_flag(cpu, FLAGS_ZERO);
		else
			i8080_clear_flag(cpu, FLAGS_ZERO);
	}
	if(mask & FLAGS_SIGN)
	{
		if(val & 0x80)
			i8080_set_flag(cpu, FLAGS_SIGN);
		else
			i8080_clear_flag(cpu, FLAGS_SIGN);
	}
}

void i8080_gensub(intel8080_t *cpu, uint8_t val)
{
	uint16_t a, b;
	// Subtract by adding with two-complement of val. Carry-flag meaning becomes inverted since we add.
	a = cpu->registers.a;
	b = 0x100 - val;

	if(i8080_check_half_carry(a, b))
		i8080_set_flag(cpu, FLAGS_H);
	else
		i8080_clear_flag(cpu, FLAGS_H);
	

	if(i8080_check_carry(a, b))
		i8080_clear_flag(cpu, FLAGS_CARRY);
	else
		i8080_set_flag(cpu, FLAGS_CARRY);

	a += b ;

	cpu->registers.a = a & 0xff;

	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
}

void i8080_compare(intel8080_t *cpu, uint8_t val)
{
	uint8_t tmp_a = cpu->registers.a;

	i8080_gensub(cpu, val);

	cpu->registers.a = tmp_a;
}

void i8080_mov(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code);
	uint8_t source = SOURCE(cpu->current_op_code);
	uint8_t val;
	uint8_t cycles;

	if(dest == MEMORY_ACCESS || source == MEMORY_ACCESS)
		cycles = CYCLES_MOV_MEM;
	else
		cycles = CYCLES_MOV_REG;


	if(cpu->decoder_step == 0)
	{
		val = i8080_regread(cpu, source);
		i8080_regwrite(cpu, dest, val);
		cpu->registers.pc++;
	}
	
	if(++cpu->decoder_step > cycles)
		cpu->decoder_step = 0;
}

void i8080_mvi(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code);
	uint8_t cycles;

	if(dest == MEMORY_ACCESS)
		cycles = CYCLES_MVI_MEM;
	else
		cycles = CYCLES_MVI_REG;

	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		i8080_regwrite(cpu, dest, cpu->data_bus);
		cpu->registers.pc++;
		break;
	}
	
	if(++cpu->decoder_step == cycles)
		cpu->decoder_step = 0;
}

void i8080_lxi(intel8080_t *cpu)
{
	uint8_t pair = RP(cpu->current_op_code);
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->decoder_state = cpu->data_bus;
		break;
	case 2:
		cpu->registers.pc++;
		i8080_pairwrite(cpu, pair, cpu->decoder_state | cpu->data_bus << 8);
		break;
	}

	if(++cpu->decoder_step == CYCLES_LXI)
		cpu->decoder_step = 0;
}

void i8080_lda(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->decoder_state = cpu->data_bus;
		break;
	case 2:
		cpu->registers.pc++;

		cpu->address_bus = cpu->decoder_state | cpu->data_bus << 8;
		i8080_mread(cpu);
		cpu->registers.a = cpu->data_bus;
		break;
	}

	if(++cpu->decoder_step == CYCLES_LDA)
		cpu->decoder_step = 0;
}

void i8080_sta(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->decoder_state = cpu->data_bus;
		break;
	case 2:
		cpu->registers.pc++;
		cpu->address_bus = cpu->decoder_state | cpu->data_bus << 8;
		cpu->data_bus = cpu->registers.a;
		i8080_mwrite(cpu);
		break;
	}

	if(++cpu->decoder_step == CYCLES_STA)
		cpu->decoder_step = 0;
}

void i8080_lhld(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->decoder_state = cpu->data_bus;
		break;
	case 2:
		cpu->registers.pc++;
		cpu->registers.hl =  *(uint16_t*)&cpu->memory[cpu->decoder_state | cpu->data_bus << 8];
		break;
	}

	if(++cpu->decoder_step == CYCLES_LHLD)
		cpu->decoder_step = 0;
}

void i8080_shld(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->decoder_state = cpu->data_bus;
		break;
	case 2:
		cpu->registers.pc++;
		*(uint16_t*)&cpu->memory[cpu->decoder_state | cpu->data_bus << 8] = cpu->registers.hl;
		break;
	}

	if(++cpu->decoder_step == CYCLES_SHLD)
		cpu->decoder_step = 0;
}

// TODO: only BC and DE allowed for indirect
void i8080_ldax(intel8080_t *cpu)
{
	uint8_t pair = RP(cpu->current_op_code);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->address_bus = i8080_pairread(cpu, pair);
		i8080_mread(cpu);
		cpu->registers.a = cpu->data_bus;
	}
	if(++cpu->decoder_step == CYCLES_LDAX)
		cpu->decoder_step = 0;
}

// TODO: only BC and DE allowed for indirect
void i8080_stax(intel8080_t *cpu)
{
	uint8_t pair = RP(cpu->current_op_code);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->address_bus = i8080_pairread(cpu, pair);
		cpu->data_bus = cpu->registers.a;
		i8080_mwrite(cpu);
	}

	if(++cpu->decoder_step == CYCLES_STAX)
		cpu->decoder_step = 0;
}

void i8080_xchg(intel8080_t *cpu)
{
	uint16_t tmp = cpu->registers.hl;

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->registers.hl = cpu->registers.de;
		cpu->registers.de = tmp;
	}
	if(++cpu->decoder_step == CYCLES_XCHG)
		cpu->decoder_step = 0;
}

void i8080_genadd(intel8080_t *cpu, uint16_t val)
{
	uint8_t a;

	a = i8080_regread(cpu, REGISTER_A);

	if(i8080_check_half_carry(a, val))
		i8080_set_flag(cpu, FLAGS_H);
	else
		i8080_clear_flag(cpu, FLAGS_H);

	if(i8080_check_carry(a, val))
		i8080_set_flag(cpu, FLAGS_CARRY);
	else
		i8080_clear_flag(cpu, FLAGS_CARRY);

	cpu->registers.a += val;

	i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
}


void i8080_add(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	uint16_t val = i8080_regread(cpu, source);
	
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		i8080_genadd(cpu, val);
	}

	if(++cpu->decoder_step == CYCLES_ADD)
		cpu->decoder_step = 0;
}

void i8080_adi(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		i8080_genadd(cpu, cpu->data_bus);
		break;
	}

	if(++cpu->decoder_step == CYCLES_ADI)
		cpu->decoder_step = 0;
}

void i8080_adc(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	uint16_t val = i8080_regread(cpu, source);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		if(cpu->registers.flags & FLAGS_CARRY)
			val++;
		i8080_genadd(cpu, val);
	}

	if(++cpu->decoder_step == CYCLES_ADC)
		cpu->decoder_step = 0;
}

void i8080_aci(intel8080_t *cpu)
{
	uint16_t val;
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		val = cpu->data_bus;
		if(cpu->registers.flags & FLAGS_CARRY)
			val++;
		i8080_genadd(cpu, val);
		break;
	}

	if(++cpu->decoder_step == CYCLES_ACI)
		cpu->decoder_step = 0;
}



void i8080_sub(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	uint8_t val = i8080_regread(cpu, source);
	
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		
		i8080_gensub(cpu, val);
	}

	if(++cpu->decoder_step == CYCLES_SUB)
		cpu->decoder_step = 0;
}

void i8080_sui(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		i8080_gensub(cpu, cpu->data_bus);
		break;
	}

	if(++cpu->decoder_step == CYCLES_SUI)
		cpu->decoder_step = 0;
}

void i8080_sbb(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);
	uint8_t val = i8080_regread(cpu, source);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		if(cpu->registers.flags & FLAGS_CARRY)
			val++;

		i8080_gensub(cpu, val);
	}

	if(++cpu->decoder_step == CYCLES_SBB)
		cpu->decoder_step = 0;
}

void i8080_sbi(intel8080_t *cpu)
{
	uint8_t val;
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		val = cpu->data_bus;
		if(cpu->registers.flags & FLAGS_CARRY)
			val++;
		i8080_gensub(cpu, val);
		break;
	}

	if(++cpu->decoder_step == CYCLES_SBI)
		cpu->decoder_step = 0;
}

void i8080_inr(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code);
	uint8_t val;
	
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		val = i8080_regread(cpu, dest);

		if(i8080_check_half_carry(val, 1))
			i8080_set_flag(cpu, FLAGS_H);
		else
			i8080_clear_flag(cpu, FLAGS_H);

		i8080_regwrite(cpu, dest, val + 1);

		i8080_update_flags(cpu, dest, FLAGS_ZERO | FLAGS_PARITY | FLAGS_SIGN | FLAGS_H);
	}

	if(++cpu->decoder_step == CYCLES_INR)
		cpu->decoder_step = 0;
}

void i8080_dcr(intel8080_t *cpu)
{
	uint8_t dest = DESTINATION(cpu->current_op_code), val;
	
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		val = i8080_regread(cpu, dest);

		if(i8080_check_half_carry(val, 0xff))
			i8080_set_flag(cpu, FLAGS_H);
		else
			i8080_clear_flag(cpu, FLAGS_H);

		i8080_regwrite(cpu, dest, val + 0xff);
		i8080_update_flags(cpu, dest, FLAGS_ZERO | FLAGS_PARITY | FLAGS_SIGN | FLAGS_H);
	}

	if(++cpu->decoder_step == CYCLES_DCR)
		cpu->decoder_step = 0;
}

void i8080_inx(intel8080_t *cpu)
{
	uint8_t rp = RP(cpu->current_op_code);
	
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		i8080_pairwrite(cpu, rp, i8080_pairread(cpu, rp) + 1);
	}

	if(++cpu->decoder_step == CYCLES_INX)
		cpu->decoder_step = 0;
}

void i8080_dcx(intel8080_t *cpu)
{
	uint8_t rp = RP(cpu->current_op_code);
	
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		i8080_pairwrite(cpu, rp, i8080_pairread(cpu, rp) - 1);
	}

	if(++cpu->decoder_step == CYCLES_DCX)
		cpu->decoder_step = 0;
}

void i8080_dad(intel8080_t *cpu)
{
	uint8_t rp = RP(cpu->current_op_code);
	
	uint32_t val = i8080_pairread(cpu, rp);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		val += i8080_pairread(cpu, PAIR_HL);

		if(val > 0xffff)
			i8080_set_flag(cpu, FLAGS_CARRY);
		else
			i8080_clear_flag(cpu, FLAGS_CARRY);
	
		i8080_pairwrite(cpu, PAIR_HL, val & 0xffff);
	}

	if(++cpu->decoder_step == CYCLES_DAD)
		cpu->decoder_step = 0;
}

void i8080_ana(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->registers.a &= i8080_regread(cpu, source);
		i8080_clear_flag(cpu, FLAGS_CARRY);
		i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
	}

	if(++cpu->decoder_step == CYCLES_ANA)
		cpu->decoder_step = 0;
}

void i8080_ani(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->registers.a &= cpu->data_bus;
		i8080_clear_flag(cpu, FLAGS_CARRY);
		i8080_clear_flag(cpu, FLAGS_H);
		i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
		break;
	}

	if(++cpu->decoder_step == CYCLES_ANI)
		cpu->decoder_step = 0;
}

void i8080_ora(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->registers.a |= i8080_regread(cpu, source);
		i8080_clear_flag(cpu, FLAGS_CARRY);
		i8080_clear_flag(cpu, FLAGS_H);
		i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
	}

	if(++cpu->decoder_step == CYCLES_ORA)
		cpu->decoder_step = 0;
}

void i8080_ori(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->registers.a |= cpu->data_bus;
		i8080_clear_flag(cpu, FLAGS_CARRY);
		i8080_clear_flag(cpu, FLAGS_H);
		i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
		break;
	}

	if(++cpu->decoder_step == CYCLES_ORI)
		cpu->decoder_step = 0;
}

void i8080_xra(intel8080_t *cpu)
{
	uint8_t source = SOURCE(cpu->current_op_code);

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->registers.a ^= i8080_regread(cpu, source);
		i8080_clear_flag(cpu, FLAGS_CARRY);
		i8080_clear_flag(cpu, FLAGS_H);
		i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
	}

	if(++cpu->decoder_step == CYCLES_XRA)
		cpu->decoder_step = 0;
}

void i8080_xri(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->registers.a ^= cpu->data_bus;
		i8080_clear_flag(cpu, FLAGS_CARRY);
		i8080_clear_flag(cpu, FLAGS_H);
		i8080_update_flags(cpu, REGISTER_A, FLAGS_ZERO | FLAGS_SIGN | FLAGS_PARITY | FLAGS_CARRY | FLAGS_H);
		break;
	}

	if(++cpu->decoder_step == CYCLES_XRI)
		cpu->decoder_step = 0;
}

void i8080_ei(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		i8080_set_flag(cpu, FLAGS_IF);
	}
	if(++cpu->decoder_step == CYCLES_EI)
		cpu->decoder_step = 0;
}

void i8080_di(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		i8080_clear_flag(cpu, FLAGS_IF);
	}
	if(++cpu->decoder_step == CYCLES_DI)
		cpu->decoder_step = 0;
}

void i8080_xthl(intel8080_t *cpu)
{
	uint16_t temp;

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;

		temp = *(uint16_t*)&cpu->memory[cpu->registers.sp];

		*(uint16_t*)&cpu->memory[cpu->registers.sp] = cpu->registers.hl;
		cpu->registers.hl = temp;
	}
	
	if(++cpu->decoder_step == CYCLES_XTHL)
		cpu->decoder_step = 0;
}

void i8080_sphl(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;

		cpu->registers.sp = cpu->registers.hl;
	}
	
	if(++cpu->decoder_step == CYCLES_SPHL)
		cpu->decoder_step = 0;
}

void i8080_in(intel8080_t *cpu)
{
	static uint8_t character = 0;
	static int counter = 0;
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		switch(cpu->data_bus)
		{
		case 0x00:
			cpu->registers.a = 0x00;
			break;
		case 0x1:
			cpu->registers.a = cpu->term_in();
			break;
		case 0x8:
			cpu->registers.a = cpu->disk_controller.disk_status();
			break;
		case 0x9:
			cpu->registers.a = cpu->disk_controller.sector();
			break;
		case 0xa:
			cpu->registers.a = cpu->disk_controller.read();
			break;
		case 0x10: // 2SIO port 1, status
			cpu->registers.a = 0x2; // bit 1 == transmit buffer empty
			character = cpu->term_in();
			if(character)
				cpu->registers.a |= 0x1;		
			break;
		case 0x11: // 2SIO port 1, read
			cpu->registers.a = character;
			break;
		case 0xff: // Front panel switches
			cpu->registers.a = 0x00;
			break;
		default:
			cpu->registers.a = 0xff;
			printf("IN PORT %x\n", cpu->data_bus);
			break;
		}
		break;
	}

	if(++cpu->decoder_step == CYCLES_IN)
		cpu->decoder_step = 0;
}

void i8080_out(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		switch(cpu->data_bus)
		{
		case 0x1:
			cpu->term_out(cpu->registers.a);
			break;
		case 0x8:
			cpu->disk_controller.disk_select(cpu->registers.a);
			break;
		case 0x9:
			cpu->disk_controller.disk_function(cpu->registers.a);
			break;
		case 0xa:
			cpu->disk_controller.write(cpu->registers.a);
			break;
		case 0x10:  // 2SIO port 1 control
			break;
		case 0x11: // 2sio port 1 write
			cpu->term_out(cpu->registers.a);
			break;
		default:
			printf("OUT PORT %x, DATA: %x\n", cpu->data_bus, cpu->registers.a);
			break;
		}
		break;
	}

	if(++cpu->decoder_step == CYCLES_OUT)
		cpu->decoder_step = 0;
}

void i8080_push(intel8080_t *cpu)
{
	uint8_t pair;
	uint16_t val;
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;

		pair = RP(cpu->current_op_code);

		if(pair == PAIR_SP)
			val = cpu->registers.af;
		else
			val = i8080_pairread(cpu, pair);

		cpu->registers.sp-=2;
		*(uint16_t*)&cpu->memory[cpu->registers.sp] = val;
	}
	
	if(++cpu->decoder_step == CYCLES_PUSH)
		cpu->decoder_step = 0;
}

void i8080_pop(intel8080_t *cpu)
{
	uint8_t pair;
	uint16_t val;
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		pair = RP(cpu->current_op_code);
		val = *(uint16_t*)&cpu->memory[cpu->registers.sp];
		cpu->registers.sp+=2;

		if(pair == PAIR_SP)
			cpu->registers.af = val;
		else
			i8080_pairwrite(cpu, pair, val);
	}
	
	if(++cpu->decoder_step == CYCLES_POP)
		cpu->decoder_step = 0;
}

void i8080_stc(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		i8080_set_flag(cpu, FLAGS_CARRY);
	}
	
	if(++cpu->decoder_step == CYCLES_STC)
		cpu->decoder_step = 0;
}

void i8080_cmc(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->registers.flags ^= FLAGS_CARRY;
	}
	
	if(++cpu->decoder_step == CYCLES_CMC)
		cpu->decoder_step = 0;
}

void i8080_rlc(intel8080_t *cpu)
{
	uint8_t high_bit;
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		high_bit = cpu->registers.a & 0x80;

		cpu->registers.a <<= 1;

		if(high_bit)
		{
			i8080_set_flag(cpu, FLAGS_CARRY);
			cpu->registers.a |= 1;
		}
		else
		{
			i8080_clear_flag(cpu, FLAGS_CARRY);
			cpu->registers.a &= ~1;
		}
		
	}
	
	if(++cpu->decoder_step == CYCLES_RAL)
		cpu->decoder_step = 0;
}

void i8080_rrc(intel8080_t *cpu)
{
	uint8_t low_bit;
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		low_bit = cpu->registers.a & 1;

		cpu->registers.a >>= 1;

		if(low_bit)
		{
			i8080_set_flag(cpu, FLAGS_CARRY);
			cpu->registers.a |= 0x80;
		}
		else
		{
			cpu->registers.a &= ~0x80;
			i8080_clear_flag(cpu, FLAGS_CARRY);
		}
		
	}
	
	if(++cpu->decoder_step == CYCLES_RAR)
		cpu->decoder_step = 0;
}

void i8080_ral(intel8080_t *cpu)
{
	uint8_t high_bit;
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		high_bit = cpu->registers.a & 0x80;

		cpu->registers.a <<= 1;

		if(cpu->registers.flags & FLAGS_CARRY)
			cpu->registers.a |= 1;
		else
			cpu->registers.a &= ~1;

		if(high_bit)
			i8080_set_flag(cpu, FLAGS_CARRY);
		else
			i8080_clear_flag(cpu, FLAGS_CARRY);
		
	}
	
	if(++cpu->decoder_step == CYCLES_RLC)
		cpu->decoder_step = 0;
}

void i8080_rar(intel8080_t *cpu)
{
	uint8_t low_bit;
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		low_bit = cpu->registers.a & 1;

		cpu->registers.a >>= 1;

		if(cpu->registers.flags & FLAGS_CARRY)
			cpu->registers.a |= 0x80;
		else
			cpu->registers.a &= ~0x80;

		if(low_bit)
			i8080_set_flag(cpu, FLAGS_CARRY);
		else
			i8080_clear_flag(cpu, FLAGS_CARRY);
		
	}
	
	if(++cpu->decoder_step == CYCLES_RRC)
		cpu->decoder_step = 0;
}

void i8080_jccc(intel8080_t *cpu)
{
	uint8_t condition;
	if(cpu->decoder_step == 0)
	{
		condition = CONDITION(cpu->current_op_code);

		if(i8080_check_condition(cpu, condition))
		{
			cpu->current_op_code = OP_JMP;
			cpu->registers.pc++;
		}
		else
		{
			cpu->registers.pc+=3;
		}
	}
	
	if(++cpu->decoder_step == CYCLES_JMP)
		cpu->decoder_step = 0;
}

void i8080_ret(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc = *(uint16_t*)&cpu->memory[cpu->registers.sp];
		cpu->registers.sp+=2;
	}
	
	if(++cpu->decoder_step == CYCLES_RET)
		cpu->decoder_step = 0;
} 

void i8080_rccc(intel8080_t *cpu)
{
	uint8_t condition;
	if(cpu->decoder_step == 0)
	{
		condition = CONDITION(cpu->current_op_code);

		if(i8080_check_condition(cpu, condition))
		{
			i8080_ret(cpu);
		}
		else
		{
			cpu->registers.pc++;
		}
	}
	
	if(++cpu->decoder_step == CYCLES_RET)
		cpu->decoder_step = 0;
} 

void i8080_jmp(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->decoder_state = cpu->data_bus;
		break;
	case 2:
		cpu->registers.pc = cpu->decoder_state | cpu->data_bus << 8;
		break;
	}

	if(++cpu->decoder_step == CYCLES_JMP)
		cpu->decoder_step = 0;
}

void i8080_rst(intel8080_t *cpu)
{
	uint8_t vec;
	if(cpu->decoder_step == 0)
	{
		vec = DESTINATION(cpu->current_op_code);

		cpu->registers.sp-=2;
		*(uint16_t*)&cpu->memory[cpu->registers.sp] = cpu->registers.pc + 1;

		cpu->registers.pc = vec*8;
	}
	
	if(++cpu->decoder_step == CYCLES_RET)
		cpu->decoder_step = 0;
} 

void i8080_call(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		cpu->decoder_state = cpu->data_bus;
		break;
	case 2:
		cpu->registers.sp-=2;
		*(uint16_t*)&cpu->memory[cpu->registers.sp] = cpu->registers.pc + 1;
		cpu->registers.pc = cpu->decoder_state | cpu->data_bus << 8;
		break;
	}

	if(++cpu->decoder_step == CYCLES_JMP)
		cpu->decoder_step = 0;
}

void i8080_cccc(intel8080_t *cpu)
{
	uint8_t condition;
	if(cpu->decoder_step == 0)
	{
		condition = CONDITION(cpu->current_op_code);

		if(i8080_check_condition(cpu, condition))
		{
			cpu->current_op_code = OP_CALL;
			cpu->registers.pc++;
		}
		else
		{
			cpu->registers.pc+=3;
		}
	}
	
	if(++cpu->decoder_step == CYCLES_CALL)
		cpu->decoder_step = 0;
}

void i8080_pchl(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc = cpu->registers.hl;
	}
	
	if(++cpu->decoder_step == CYCLES_PCHL)
		cpu->decoder_step = 0;
}


void i8080_nop(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
	}

	if(++cpu->decoder_step == CYCLES_NOP)
		cpu->decoder_step = 0;
}

void i8080_cma(intel8080_t *cpu)
{
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		cpu->registers.a = ~cpu->registers.a;
	}

	if(++cpu->decoder_step == CYCLES_CMA)
		cpu->decoder_step = 0;
}

void i8080_cmp(intel8080_t *cpu)
{
	uint8_t reg;
	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		reg = SOURCE(cpu->current_op_code);
		i8080_compare(cpu, i8080_regread(cpu, reg));
	}

	if(++cpu->decoder_step == CYCLES_CMP)
		cpu->decoder_step = 0;
}

void i8080_cpi(intel8080_t *cpu)
{
	switch(cpu->decoder_step)
	{
	case 0:
		cpu->registers.pc++;
		break;
	case 1:
		cpu->registers.pc++;
		i8080_compare(cpu, cpu->data_bus);
		break;
	}

	if(++cpu->decoder_step == CYCLES_CPI)
		cpu->decoder_step = 0;
}

void i8080_fetch_next_op(intel8080_t *cpu)
{
	cpu->address_bus = cpu->registers.pc;
	i8080_mread(cpu);
}

void i8080_daa(intel8080_t *cpu)
{
	uint8_t val, add = 0;

	if(cpu->decoder_step == 0)
	{
		cpu->registers.pc++;
		val = i8080_regread(cpu, REGISTER_A);

		if((val & 0xf) > 9 || cpu->registers.flags & FLAGS_H)
			add += 0x06;

		val += add;

		if(((val & 0xf0) >> 4) > 9 || cpu->registers.flags & FLAGS_CARRY)
			add += 0x60;

		i8080_genadd(cpu, add);
	}

	if(++cpu->decoder_step == CYCLES_DAA)
		cpu->decoder_step = 0;
}

void i8080_cycle(intel8080_t *cpu)
{
	uint8_t op_code;

	if(cpu->decoder_step)
		op_code = cpu->current_op_code;
	else
		op_code = cpu->current_op_code = cpu->data_bus;
	
	if(ISOP(MOV, op_code))
		i8080_mov(cpu);
	else if(ISOP(MVI, op_code))
		i8080_mvi(cpu);
	else if(ISOP(LXI, op_code))
		i8080_lxi(cpu);
	else if(ISOP(LDA, op_code))
		i8080_lda(cpu);
	else if(ISOP(STA, op_code))
		i8080_sta(cpu);
	else if(ISOP(LHLD, op_code))
		i8080_lhld(cpu);
	else if(ISOP(SHLD, op_code))
		i8080_shld(cpu);
	else if(ISOP(LDAX, op_code))
		i8080_ldax(cpu);
	else if(ISOP(STAX, op_code))
		i8080_stax(cpu);
	else if(ISOP(XCHG, op_code))
		i8080_xchg(cpu);
	else if(ISOP(ADD, op_code))
		i8080_add(cpu);
	else if(ISOP(ADI, op_code))
		i8080_adi(cpu);
	else if(ISOP(ADC, op_code))
		i8080_adc(cpu);
	else if(ISOP(ACI, op_code))
		i8080_aci(cpu);
	else if(ISOP(SUB, op_code))
		i8080_sub(cpu);
	else if(ISOP(SUI, op_code))
		i8080_sui(cpu);
	else if(ISOP(SBB, op_code))
		i8080_sbb(cpu);
	else if(ISOP(SBI, op_code))
		i8080_sbi(cpu);
	else if(ISOP(INR, op_code))
		i8080_inr(cpu);
	else if(ISOP(DCR, op_code))
		i8080_dcr(cpu);
	else if(ISOP(INX, op_code))
		i8080_inx(cpu);
	else if(ISOP(DCX, op_code))
		i8080_dcx(cpu);
	else if(ISOP(DAD, op_code))
		i8080_dad(cpu);	
	else if(ISOP(ANA, op_code))
		i8080_ana(cpu);
	else if(ISOP(ANI, op_code))
		i8080_ani(cpu);
	else if(ISOP(ORA, op_code))
		i8080_ora(cpu);
	else if(ISOP(ORI, op_code))
		i8080_ori(cpu);
	else if(ISOP(XRA, op_code))
		i8080_xra(cpu);
	else if(ISOP(XRI, op_code))
		i8080_xri(cpu);
	else if(ISOP(EI, op_code))
		i8080_ei(cpu);
	else if(ISOP(DI, op_code))
		i8080_di(cpu);
	else if(ISOP(XTHL, op_code))
		i8080_xthl(cpu);
	else if(ISOP(SPHL, op_code))
		i8080_sphl(cpu);
	else if(ISOP(IN, op_code))
		i8080_in(cpu);
	else if(ISOP(OUT, op_code))
		i8080_out(cpu);
	else if(ISOP(PUSH, op_code))
		i8080_push(cpu);
	else if(ISOP(POP, op_code))
		i8080_pop(cpu);
	else if(ISOP(RLC, op_code))
		i8080_rlc(cpu);
	else if(ISOP(RRC, op_code))
		i8080_rrc(cpu);
	else if(ISOP(RAL, op_code))
		i8080_ral(cpu);
	else if(ISOP(RAR, op_code))
		i8080_rar(cpu);
	else if(ISOP(JCCC, op_code))
		i8080_jccc(cpu);
	else if(ISOP(JMP, op_code) || op_code == 0xcb)
		i8080_jmp(cpu);
	else if(ISOP(NOP, op_code) || op_code == 0x10 || op_code == 0x08|| op_code == 0x18 || op_code == 0x20 || op_code == 0x28 || op_code == 0x30 || op_code == 0x38)
		i8080_nop(cpu);
	else if(ISOP(RET, op_code) || op_code == 0xD9)
		i8080_ret(cpu);
	else if(ISOP(RCCC, op_code))
		i8080_rccc(cpu);
	else if(ISOP(CALL, op_code) || op_code == 0xFD)
		i8080_call(cpu);
	else if(ISOP(RST, op_code))
		i8080_rst(cpu);
	else if(ISOP(CMP, op_code))
		i8080_cmp(cpu);
	else if(ISOP(CPI, op_code))
		i8080_cpi(cpu);
	else if(ISOP(CCCC, op_code))
		i8080_cccc(cpu);
	else if(ISOP(STC, op_code))
		i8080_stc(cpu);
	else if(ISOP(CMC, op_code))
		i8080_cmc(cpu);
	else if(ISOP(CMA, op_code))
		i8080_cma(cpu);
	else if(ISOP(PCHL, op_code))
		i8080_pchl(cpu);
	else if(ISOP(DAA, op_code))
		i8080_daa(cpu);
	else
	{
		printf("Unknown opcode!\n");
		while(1);
	}
	

	i8080_fetch_next_op(cpu);
}

void i8080_sync(intel8080_t *cpu)
{
	while(cpu->decoder_step)
		i8080_cycle(cpu);
}



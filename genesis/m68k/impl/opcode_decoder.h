#ifndef __M68K_OPCODE_DECODER_H__
#define __M68K_OPCODE_DECODER_H__

#include <string_view>
#include "instruction_type.h"


namespace genesis::m68k
{

struct mask_inst_pair
{
	std::string_view inst_template;
	inst_type inst;
};

/* Placeholders:
	- 0/1 -- self-explanatory
	- _ -- either 1 or 0
	- sz -- any of 00 01 10
*/

const constexpr mask_inst_pair opcodes[] =
{
	/* ADD */
	{ "1101____sz______", inst_type::ADD },
	{ "1101____11______", inst_type::ADDA },
	{ "00000110sz______", inst_type::ADDI },
	{ "0101___0sz______", inst_type::ADDQ },
	{ "1101___1sz00____", inst_type::ADDX },
	{ "0000001000111100", inst_type::ANDItoCCR },
	{ "0000001001111100", inst_type::ANDItoSR },

	/* SUB */
	{ "1001____sz______", inst_type::SUB },
	{ "1001____11______", inst_type::SUBA },
	{ "00000100sz______", inst_type::SUBI },
	{ "0101___1sz______", inst_type::SUBQ },
	{ "1001___1sz00____", inst_type::SUBX },

	/* AND */
	{ "1100____sz______", inst_type::AND },
	{ "00000010sz______", inst_type::ANDI },

	/* OR */
	{ "1000____sz______", inst_type::OR },
	{ "00000000sz______", inst_type::ORI },
	{ "0000000000111100", inst_type::ORItoCCR },
	{ "0000000001111100", inst_type::ORItoSR },

	/* EOR */
	{ "1011___1sz______", inst_type::EOR },
	{ "00001010sz______", inst_type::EORI },
	{ "0000101000111100", inst_type::EORItoCCR },
	{ "0000101001111100", inst_type::EORItoSR },

	/* CMP */
	{ "1011___0sz______", inst_type::CMP },
	{ "1011____11______", inst_type::CMPA },
	{ "1011___1sz001___", inst_type::CMPM },
	{ "00001100sz______", inst_type::CMPI },

	/* NEG */
	{ "01000100sz______", inst_type::NEG },
	{ "01000000sz______", inst_type::NEGX },

	/* NOT */
	{ "01000110sz______", inst_type::NOT },

	/* NOP */
	{ "0100111001110001", inst_type::NOP },

	/* MOVE */
	{ "0001____________", inst_type::MOVE },
	{ "0011____________", inst_type::MOVE },
	{ "0010____________", inst_type::MOVE },
	{ "0111___0________", inst_type::MOVEQ },
	{ "001____001______", inst_type::MOVEA },
	{ "01001_001_______", inst_type::MOVEM },
	{ "0000___1__001___", inst_type::MOVEP },
	{ "0100000011______", inst_type::MOVEfromSR },
	{ "0100011011______", inst_type::MOVEtoSR },
	{ "0100010011______", inst_type::MOVEtoCCR },
	{ "010011100110____", inst_type::MOVE_USP },

	/* ASL/ASR/ROL/ROR/LSL/LSR */
	{ "1110____sz_00___", inst_type::ASLRreg },
	{ "1110000_11______", inst_type::ASLRmem },
	{ "1110____sz_11___", inst_type::ROLRreg },
	{ "1110011_11______", inst_type::ROLRmem },
	{ "1110____sz_01___", inst_type::LSLRreg },
	{ "1110001_11______", inst_type::LSLRmem },
	{ "1110____sz_10___", inst_type::ROXLRreg },
	{ "1110010_11______", inst_type::ROXLRmem },

	/* TST */
	{ "01001010sz______", inst_type::TST },

	/* CLR */
	{ "01000010sz______", inst_type::CLR },

	/* MULU/MULS */
	{ "1100___011______", inst_type::MULU },
	{ "1100___111______", inst_type::MULS },
};


class opcode_decoder
{
public:
	opcode_decoder() = delete;

	static m68k::inst_type decode(std::uint16_t opcode);
};

}

#endif // __M68K_OPCODE_DECODER_H__

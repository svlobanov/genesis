#ifndef __M68K_OPERATIONS_HPP__
#define __M68K_OPERATIONS_HPP__

#include "cpu_flags.hpp"
#include "exception.hpp"

#include "instruction_type.h"
#include "m68k/cpu_registers.hpp"
#include "ea_decoder.hpp"


namespace genesis::m68k
{

class operations
{
public:
	operations() = delete;

	template<class T1, class T2>
	static std::uint32_t add(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return add(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t adda(T1 src, T2 dest, std::uint8_t size, status_register&)
	{
		if(size == 2)
			return (std::int16_t)value(src, size) + value(dest, 4 /* always long word */);
		return value(src, size) + value(dest, size);
	}

	template<class T1, class T2>
	static std::uint32_t addx(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return addx(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t sub(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return sub(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t subx(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return subx(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t cmp(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		// cmp like sub but does not change the result
		std::uint8_t old_x = sr.X;
		sub(value(a, size), value(b, size), size, sr);
		sr.X = old_x; // in case of CMP X is not affected
		return value(a, size);
	}

	template<class T1, class T2>
	static std::uint32_t cmpa(T1 src, T2 dest, std::uint8_t size, status_register& sr)
	{
		std::uint32_t b = value(dest, 4 /* always long word */);
		std::uint32_t a;
		if(size == 2)
			a = (std::int32_t)(std::int16_t)value(src, size);
		else
			a = value(src, size);
		return cmp(b, a, 4, sr);
	}

	template<class T1, class T2>
	static std::uint32_t suba(T1 src, T2 dest, std::uint8_t size, status_register&)
	{
		if(size == 2)
			return value(dest, 4 /* always long word */) - (std::int16_t)value(src, size);
		return value(dest, size) - value(src, size);
	}

	template<class T1, class T2>
	static std::uint32_t and_op(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return and_op(value(a, size), value(b, size), size, sr);
	}

	static void andi_to_ccr(std::uint8_t src, std::uint16_t& SR)
	{
		std::uint16_t src_mask = 0xFFE0 | src;
		SR = SR & src_mask;
	}

	static void andi_to_sr(std::uint16_t src, std::uint16_t& SR)
	{
		SR = SR & src;
	}

	template<class T1, class T2>
	static std::uint32_t or_op(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return or_op(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t eor(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return eor(value(a, size), value(b, size), size, sr);
	}

	template<class T1>
	static std::uint32_t neg(T1 a, std::uint8_t size, status_register& sr)
	{
		return sub(0, value(a, size), size, sr);
	}

	template<class T1>
	static std::uint32_t negx(T1 a, std::uint8_t size, status_register& sr)
	{
		return subx(0, value(a, size), size, sr);
	}

	template<class T1>
	static std::uint32_t not_op(T1 a, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = value(~value(a, size), size);
		sr.N = neg_flag(res, size);
		sr.V = sr.C = 0;
		sr.Z = res == 0;
		return res;
	}

	template<class T1>
	static std::uint32_t move(T1 src, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = value(src, size);
		sr.N = neg_flag(res, size);
		sr.Z = res == 0;
		sr.V = sr.C = 0;
		return res;
	}

	template<class T1>
	static std::uint32_t movea(T1 src, std::uint8_t size)
	{
		if(size == size_type::LONG)
			return value(src, size);
		return sign_extend(value(src, size));
	}

	template<class T1>
	static std::uint16_t move_to_sr(T1 src)
	{
		const std::uint16_t mask = 0b1010011100011111; // I've got no idea why T/M bits are cleared here
		return value(src, size_type::WORD) & mask;
	}

	template<class T1>
	static std::uint16_t move_to_ccr(T1 src, std::uint16_t sr)
	{
		std::uint8_t low_byte = value(src, size_type::BYTE) & 0b11111;
		return (sr & 0xFF00) | low_byte;
	}

	/* helpers */
	template<class T1, class T2>
	static std::uint32_t alu(inst_type inst, T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		switch (inst)
		{
		case inst_type::ADD:
		case inst_type::ADDI:
		case inst_type::ADDQ:
			return operations::add(a, b, size, sr);

		case inst_type::ADDA:
			return operations::adda(a, b, size, sr);

		case inst_type::ADDX:
			return operations::addx(a, b, size, sr);

		case inst_type::SUB:
		case inst_type::SUBI:
		case inst_type::SUBQ:
			return operations::sub(a, b, size, sr);

		case inst_type::SUBA:
			return operations::suba(a, b, size, sr);

		case inst_type::SUBX:
			return operations::subx(a, b, size, sr);

		case inst_type::AND:
		case inst_type::ANDI:
			return operations::and_op(a, b, size, sr);

		case inst_type::OR:
		case inst_type::ORI:
			return operations::or_op(a, b, size, sr);

		case inst_type::EOR:
		case inst_type::EORI:
			return operations::eor(a, b, size, sr);

		case inst_type::CMP:
		case inst_type::CMPI:
		case inst_type::CMPM:
			return operations::cmp(a, b, size, sr);

		case inst_type::CMPA:
			return operations::cmpa(a, b, size, sr);

		default: throw internal_error();
		}
	}

	template<class T1>
	static std::uint32_t alu(inst_type inst, T1 a, std::uint8_t size, status_register& sr)
	{
		switch (inst)
		{
		case inst_type::NEG:
			return neg(a, size, sr);
		case inst_type::NEGX:
			return negx(a, size, sr);
		case inst_type::NOT:
			return not_op(a, size, sr);
		case inst_type::MOVE:
			return move(a, size, sr);

		default: throw internal_error();
		}
	}

	static std::uint32_t sign_extend(std::uint16_t val)
	{
		return std::int32_t(std::int16_t(val));
	}

private:
	static std::uint32_t add(std::uint32_t a, std::uint32_t b, std::uint8_t x, std::uint8_t size)
	{
		if(size == 1) return std::uint8_t(a + b + x);
		if(size == 2) return std::uint16_t(a + b + x);
		return a + b + x;
	}

	static std::uint32_t add(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = add(a, b, 0, size);
		set_carry_and_overflow_flags(a, b, 0, size, sr);
		sr.X = sr.C;
		sr.Z = res == 0;
		sr.N = neg_flag(res, size);
		return res;
	}

	static std::uint32_t addx(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = add(a, b, sr.X, size);
		set_carry_and_overflow_flags(a, b, sr.X, size, sr);
		sr.X = sr.C;
		if(res != 0) sr.Z = 0;
		sr.N = neg_flag(res, size);
		return res;
	}

	static std::uint32_t sub(std::uint32_t a, std::uint32_t b, std::uint8_t x, std::uint8_t size)
	{
		if(size == 1) return std::uint8_t(a - b - x);
		if(size == 2) return std::uint16_t(a - b - x);
		return a - b - x;
	}

	static std::uint32_t sub(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = sub(a, b, 0, size);
		set_borrow_and_overflow_flags(a, b, 0, size, sr);
		sr.X = sr.C;
		sr.Z = res == 0;
		sr.N = neg_flag(res, size);
		return res;
	}

	static std::uint32_t subx(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = sub(a, b, sr.X, size);
		set_borrow_and_overflow_flags(a, b, sr.X, size, sr);
		sr.X = sr.C;
		if(res != 0) sr.Z = 0;
		sr.N = neg_flag(res, size);
		return res;
	}

	static std::uint32_t and_op(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = a & b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static std::uint32_t or_op(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = a | b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static std::uint32_t eor(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = a ^ b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static void set_carry_and_overflow_flags(std::uint32_t a, std::uint32_t b, std::uint8_t x,
		std::uint8_t size, status_register& sr)
	{
		if(size == 1)
		{
			sr.V = cpu_flags::overflow_add<std::int8_t>(a, b, x);
			sr.C = cpu_flags::carry<std::uint8_t>(a, b, x);
		}
		else if(size == 2)
		{
			sr.V = cpu_flags::overflow_add<std::int16_t>(a, b, x);
			sr.C = cpu_flags::carry<std::uint16_t>(a, b, x);
		}
		else
		{
			sr.V = cpu_flags::overflow_add<std::int32_t>(a, b, x);
			sr.C = cpu_flags::carry<std::uint32_t>(a, b, x);
		}
	}

	static void set_borrow_and_overflow_flags(std::uint32_t a, std::uint32_t b, std::uint8_t x,
		std::uint8_t size, status_register& sr)
	{
		if(size == 1)
		{
			sr.V = cpu_flags::overflow_sub<std::int8_t>(a, b, x);
			sr.C = cpu_flags::borrow<std::uint8_t>(a, b, x);
		}
		else if(size == 2)
		{
			sr.V = cpu_flags::overflow_sub<std::int16_t>(a, b, x);
			sr.C = cpu_flags::borrow<std::uint16_t>(a, b, x);
		}
		else
		{
			sr.V = cpu_flags::overflow_sub<std::int32_t>(a, b, x);
			sr.C = cpu_flags::borrow<std::uint32_t>(a, b, x);
		}
	}

	static void set_logical_flags(std::uint32_t res, std::uint8_t size, status_register& sr)
	{
		sr.C = sr.V = 0;
		sr.Z = res == 0;
		sr.N = neg_flag(res, size);
	}

	static std::uint8_t neg_flag(std::uint32_t val, std::uint8_t size)
	{
		if(size == 1) return std::int8_t(val) < 0;
		if(size == 2) return std::int16_t(val) < 0;
		return std::int32_t(val) < 0;
	}

public:
	static std::uint32_t value(data_register reg, std::uint8_t size)
	{
		if(size == 1)
			return reg.B;
		else if(size == 2)
			return reg.W;
		else
			return reg.LW;
	}

	static std::uint32_t value(address_register reg, std::uint8_t size)
	{
		if(size == 2)
			return reg.W;
		else if(size == 4)
			return reg.LW;

		throw internal_error();
	}

	static std::uint32_t value(std::uint32_t val, std::uint8_t size)
	{
		if(size == 1)
			return val & 0xFF;
		if (size == 2)
			return val & 0xFFFF;
		return val;
	}

	static std::uint32_t value(operand& op, std::uint8_t size)
	{
		if(op.is_imm()) return value(op.imm(), size);
		if(op.is_pointer()) return value(op.pointer().value(), size);
		if(op.is_data_reg()) return value(op.data_reg(), size);
		if(op.is_addr_reg()) return value(op.addr_reg(), size);

		throw internal_error();
	}
};

}

#endif // __M68K_OPERATIONS_HPP__

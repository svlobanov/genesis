#ifndef __M68K_EA_DECODER_HPP__
#define __M68K_EA_DECODER_HPP__

#include "bus_scheduler.h"
#include "endian.hpp"
#include "m68k/cpu_registers.hpp"
#include "size_type.h"

#include <cstdint>


namespace genesis::m68k
{

enum class addressing_mode
{
	// Data Register Direct
	// Dn
	data_reg,

	// Address Register Direct
	// An
	addr_reg,

	// Address Register Indirect
	// (An)
	indir,

	// Address Register Indirect with Postincrement
	// (An)+
	postinc,

	// Address Register Indirect with Predecrement
	// –(An)
	predec,

	// Address Register Indirect with Displacement
	// (d16, An)
	disp_indir,

	// Address Register Indirect with Index
	// (d8, An, Xn)
	index_indir,

	// Absolute Short
	// (xxx).W
	abs_short,

	// Absolute Long
	// (xxx).L
	abs_long,

	// Program Counter Indirect with Displacement
	// (d8, PC)
	disp_pc,

	// Program Counter Indirect with Index
	// (d16, PC, Xn)
	index_pc,

	// Immediate
	// #<data>
	imm,

	unknown
};

class operand
{
public:
	struct raw_pointer
	{
		raw_pointer(std::uint32_t address) : address(address)
		{
		}
		raw_pointer(std::uint32_t address, std::uint32_t value) : address(address), _value(value)
		{
		}

		std::uint32_t address;

		bool has_value() const
		{
			return _value.has_value();
		}

		std::uint32_t value() const
		{
			if(!has_value())
				throw internal_error();

			return _value.value();
		}

	private:
		std::optional<std::uint32_t> _value;
	};

public:
	operand(address_register& _addr_reg, size_type size)
		: _addr_reg(_addr_reg), _size(size), _mode(addressing_mode::addr_reg)
	{
	}

	operand(data_register& _data_reg, size_type size)
		: _data_reg(_data_reg), _size(size), _mode(addressing_mode::data_reg)
	{
	}

	operand(std::uint32_t _imm, size_type size) : _imm(_imm), _size(size), _mode(addressing_mode::imm)
	{
	}

	operand(raw_pointer ptr, size_type size, addressing_mode mode) : _ptr(ptr), _size(size), _mode(mode)
	{
	}

	bool is_addr_reg() const
	{
		return _addr_reg.has_value();
	}
	bool is_data_reg() const
	{
		return _data_reg.has_value();
	}
	bool is_imm() const
	{
		return _imm.has_value();
	}
	bool is_pointer() const
	{
		return _ptr.has_value();
	}

	addressing_mode mode() const
	{
		return _mode;
	}

	address_register& addr_reg()
	{
		if(!is_addr_reg())
			throw internal_error();

		return _addr_reg.value().get();
	}

	data_register& data_reg()
	{
		if(!is_data_reg())
			throw internal_error();

		return _data_reg.value().get();
	}

	std::uint32_t imm() const
	{
		if(!is_imm())
			throw internal_error();

		return _imm.value();
	}

	raw_pointer pointer() const
	{
		if(!is_pointer())
			throw internal_error();

		return _ptr.value();
	}

	size_type size() const
	{
		return _size;
	}

private:
	std::optional<std::reference_wrapper<address_register>> _addr_reg;
	std::optional<std::reference_wrapper<data_register>> _data_reg;
	std::optional<std::uint32_t> _imm;
	std::optional<raw_pointer> _ptr;
	size_type _size;
	addressing_mode _mode;
};


// effective address decoder
class ea_decoder
{
public:
	enum class flags
	{
		none = 0,
		no_read = 2,
		no_prefetch = 4,
	};

public:
	ea_decoder(cpu_registers& regs, bus_scheduler& scheduler) : regs(regs), scheduler(scheduler)
	{
	}

	bool ready() const
	{
		return res.has_value();
	}

	operand result()
	{
		if(!ready())
			throw internal_error();

		return res.value();
	}

	void reset()
	{
		res.reset();
	}

	void schedule_decoding(std::uint8_t ea, size_type size, flags flags = flags::none)
	{
		if(!scheduler.is_idle())
		{
			// due to scheduler restrictions we cannot schedule decoding if smth is already scheduled
			// ('cause scheduled operation can change registers and thereby affect the result)
			throw internal_error();
		}

		res.reset();
		std::uint8_t reg = ea & 0x7;

		this->flags = flags;
		this->size = size;
		this->reg = reg;
		mode = decode_mode(ea);

		schedule_decoding(mode, reg, size);
	}

	constexpr static addressing_mode decode_mode(std::uint8_t ea)
	{
		std::uint8_t mode = (ea >> 3) & 0x7;
		std::uint8_t reg = ea & 0x7;

		switch(mode)
		{
		case 0b000:
			return addressing_mode::data_reg;

		case 0b001:
			return addressing_mode::addr_reg;

		case 0b010:
			return addressing_mode::indir;

		case 0b011:
			return addressing_mode::postinc;

		case 0b100:
			return addressing_mode::predec;

		case 0b101:
			return addressing_mode::disp_indir;

		case 0b110:
			return addressing_mode::index_indir;

		case 0b111: {
			switch(reg)
			{
			case 0b000:
				return addressing_mode::abs_short;

			case 0b001:
				return addressing_mode::abs_long;

			case 0b010:
				return addressing_mode::disp_pc;

			case 0b011:
				return addressing_mode::index_pc;

			case 0b100:
				return addressing_mode::imm;

			default:
				return addressing_mode::unknown;
			}

		default:
			return addressing_mode::unknown;
		}
		}
	}

private:
	void schedule_decoding(addressing_mode mode, std::uint8_t reg, size_type size)
	{
		switch(mode)
		{
		case addressing_mode::data_reg:
			decode_data_reg(reg, size);
			break;

		case addressing_mode::addr_reg:
			decode_addr_reg(reg, size);
			break;

		case addressing_mode::indir:
			decode_indir(reg, size);
			break;

		case addressing_mode::postinc:
			decode_postinc(reg, size);
			break;

		case addressing_mode::predec:
			decode_predec(reg, size);
			break;

		case addressing_mode::disp_indir:
			decode_disp_indir(reg, size);
			break;

		case addressing_mode::index_indir:
			decode_index_indir(reg, size);
			break;

		case addressing_mode::abs_short:
			decode_abs_short(size);
			break;

		case addressing_mode::abs_long:
			decode_abs_long(size);
			break;

		case addressing_mode::disp_pc:
			decode_disp_pc(size);
			break;

		case addressing_mode::index_pc:
			decode_index_pc(size);
			break;

		case addressing_mode::imm:
			decode_imm(size);
			break;

		default:
			throw internal_error();
		}
	}

private:
	/* immediately decoding */
	void decode_data_reg(std::uint8_t reg, size_type size)
	{
		res = {regs.D(reg), size};
	}

	void decode_addr_reg(std::uint8_t reg, size_type size)
	{
		res = {regs.A(reg), size};
	}

	// Address Register Indirect Mode
	void decode_indir(std::uint8_t reg, size_type size)
	{
		schedule_read_and_save(regs.A(reg).LW, size);
	}

	// Address Register Indirect with Postincrement Mode
	void decode_postinc(std::uint8_t reg, size_type size)
	{
		if(flags == flags::none)
		{
			// TODO: by common logic it should be below read operation,
			// however, external tests expects too see this logic
			scheduler.inc_addr_reg(reg, size);
		}

		schedule_read_and_save(regs.A(reg).LW, size);
	}

	// Address Register Indirect with Predecrement Mode
	void decode_predec(std::uint8_t reg, size_type size)
	{
		if(flags == flags::none)
		{
			scheduler.wait(2);
			regs.dec_addr(reg, size);
		}

		schedule_read_and_save(regs.A(reg).LW, size);
	}

	// Address Register Indirect with Displacement Mode
	void decode_disp_indir(std::uint8_t reg, size_type size)
	{
		schedule_prefetch_irc();

		std::uint32_t ptr = (std::int32_t)regs.A(reg).LW + std::int32_t((std::int16_t)regs.IRC);
		schedule_read_and_save(ptr, size);
	}

	// Address Register Indirect with Index (8-Bit Displacement) Mode
	void decode_index_indir(std::uint8_t reg, size_type size)
	{
		if(no_prefetch())
		{
			scheduler.wait(6); // decoding takes 6 cycles
		}
		else
		{
			scheduler.wait(2);
			scheduler.read_imm(size_type::WORD);
		}

		std::uint32_t ptr = dec_brief_reg(regs.A(reg).LW, regs);
		schedule_read_and_save(ptr, size);
	}

	// Absolute Short Addressing Mode
	void decode_abs_short(size_type size)
	{
		schedule_prefetch_irc();
		schedule_read_and_save((std::int16_t)regs.IRC, size);
	}

	// Absolute Long Addressing Mode
	void decode_abs_long(size_type size)
	{
		this->size = size;
		auto flags = no_prefetch() ? read_imm_flags::no_prefetch : read_imm_flags::do_prefetch;
		scheduler.read_imm(size_type::LONG, flags,
						   [this](std::uint32_t imm, size_type) { schedule_read_and_save(imm, this->size); });
	}

	// Program Counter Indirect with Displacement Mode
	void decode_disp_pc(size_type size)
	{
		schedule_prefetch_irc();

		std::uint32_t ptr = regs.PC + (std::int16_t)regs.IRC;
		schedule_read_and_save(ptr, size);
	}

	// Program Counter Indirect with Index (8-Bit Displacement) Mode
	void decode_index_pc(size_type size)
	{
		if(no_prefetch())
		{
			scheduler.wait(6); // decoding takes 6 cycles
		}
		else
		{
			scheduler.wait(2);
			scheduler.read_imm(size_type::WORD);
		}

		std::uint32_t ptr = dec_brief_reg(regs.PC, regs);
		schedule_read_and_save(ptr, size);
	}

	// Immediate Data
	void decode_imm(size_type size)
	{
		auto flags = no_prefetch() ? read_imm_flags::no_prefetch : read_imm_flags::do_prefetch;
		scheduler.read_imm(size, flags, [this](std::uint32_t imm, size_type size) { res = {imm, size}; });
	}


private:
	/* helper methods */
	void schedule_prefetch_irc()
	{
		if(no_prefetch())
			scheduler.wait(2); // 2 cycles takes to calc address
		else
			scheduler.read_imm(size_type::WORD);
	}

	void schedule_read_and_save(std::uint32_t addr, size_type size)
	{
		if(no_read())
		{
			res = {operand::raw_pointer(addr), size, mode};
		}
		else
		{
			ptr = addr;
			scheduler.read(addr, size, [this](std::uint32_t data, size_type size) {
				res = {operand::raw_pointer(ptr, data), size, mode};
			});
		}
	}

	/* flags helpers */
	bool flag_set(flags flag) const
	{
		std::uint8_t raw_flag = std::uint8_t(flag);
		std::uint8_t raw_flags = std::uint8_t(flags);

		return (raw_flags & raw_flag) > 0;
	}

	bool no_prefetch() const
	{
		return flag_set(flags::no_prefetch);
	}

	bool no_read() const
	{
		return flag_set(flags::no_read);
	}

private:
	struct brief_ext
	{
		brief_ext(std::uint16_t raw)
		{
			displacement = endian::lsb(raw);

			std::uint8_t msb = endian::msb(raw);
			wl = (msb >> 3) & 0b1;
			reg = (msb >> 4) & 0b111;
			da = (msb >> 7) & 0b1;
		}

		std::uint8_t displacement;
		// std::uint8_t reserved : 3;
		std::uint8_t wl : 1;
		std::uint8_t reg : 3;
		std::uint8_t da : 1;
	};

public:
	static std::uint32_t dec_brief_reg(std::uint32_t base, cpu_registers& regs)
	{
		brief_ext ext(regs.IRC);
		base += (std::int32_t)(std::int8_t)ext.displacement;

		if(ext.wl)
			base += ext.da ? regs.A(ext.reg).LW : regs.D(ext.reg).LW;
		else
			base += (std::int16_t)(ext.da ? regs.A(ext.reg).W : regs.D(ext.reg).W);

		return base;
	}

private:
	cpu_registers& regs;
	bus_scheduler& scheduler;

	std::optional<operand> res;

	size_type size{};
	std::uint8_t reg{};
	addressing_mode mode{};
	std::uint32_t ptr{};
	flags flags = flags::none;
};


constexpr enum ea_decoder::flags operator|(const enum ea_decoder::flags selfValue, const enum ea_decoder::flags inValue)
{
	return (enum ea_decoder::flags)(std::uint8_t(selfValue) | std::uint8_t(inValue));
}

} // namespace genesis::m68k

#endif //__M68K_EA_DECODER_HPP__

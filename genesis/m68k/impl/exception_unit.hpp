#ifndef __M68K_EXCEPTION_UNIT_HPP__
#define __M68K_EXCEPTION_UNIT_HPP__

#include "base_unit.h"
#include "exception_manager.h"
#include "instruction_unit.hpp"

#include "exception.hpp"


namespace genesis::m68k
{

class exception_unit : public base_unit
{
private:
	enum ex_state
	{
		IDLE,
		EXECUTING,
	};

public:
	exception_unit(m68k::cpu_registers& regs, exception_manager& exman, m68k::bus_scheduler& scheduler,
		std::function<void()> __abort_execution)
		: base_unit(regs, scheduler), exman(exman), __abort_execution(__abort_execution)
	{
		if(__abort_execution == nullptr)
			throw std::invalid_argument("__abort_execution");

		reset();
	}

	void reset() override
	{
		state = IDLE;
		curr_ex = exception_type::none;
		base_unit::reset();
	}

	bool has_work() const
	{
		/* We should process these exceptions as soon as possible */
		if(exman.is_raised(exception_type::address_error))
			return true;

		if(exman.is_raised(exception_type::bus_error))
			return true;

		if(exman.is_raised(exception_type::trap))
			return true;

		return false;
	}

protected:
	exec_state on_executing() override
	{
		switch (state)
		{
		case IDLE:
			accept_exception();
			state = EXECUTING;
			[[fallthrough]];

		case EXECUTING:
			// TODO: check if another exception is rised during executing the curr exception
			return exec();

		default:
			throw internal_error();
		}
	}

private:
	exec_state exec()
	{
		switch (curr_ex)
		{
		case exception_type::address_error:
		case exception_type::bus_error:
			return address_error();
		
		case exception_type::trap:
			return trap();
	
		default: throw internal_error();
		}
	}

	void accept_exception()
	{
		if(exman.is_raised(exception_type::address_error))
		{
			curr_ex = exception_type::address_error;
			addr_error = exman.accept_address_error();
			abort_execution();
		}
		else if(exman.is_raised(exception_type::bus_error))
		{
			curr_ex = exception_type::bus_error;
			addr_error = exman.accept_bus_error();
			abort_execution();
		}
		else if(exman.is_raised(exception_type::trap))
		{
			// TODO: do we need to abort / or wait for smth?
			curr_ex = exception_type::trap;
			trap_vector = exman.accept_trap();
		}
		else
		{
			throw not_implemented();
		}
	}

	/* Sequence of actions
	 * 1. Push PC
	 * 2. Push SR
	 * 3. Push Instruction Register (IRD)
	 * 4. Push address
	 * 5. Info word
	*/
	exec_state address_error()
	{
		correct_pc();
		scheduler.wait(4 - 1);

		// PUSH PC LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, addr_error.PC & 0xFFFF, size_type::WORD);

		// PUSH SR
		// note, for some reason we first push SR, then PC HIGH
		scheduler.write(regs.SSP.LW - 4, regs.SR, size_type::WORD);

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;

		// PUSH PC HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, addr_error.PC >> 16, size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		// PUSH IRD
		regs.SSP.LW -= 2;
		// TODO: IRD not always contains the current instruction
		scheduler.write(regs.SSP.LW, regs.SIRD, size_type::WORD);

		// PUSH address LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, addr_error.address & 0xFFFF, size_type::WORD);

		// PUSH status word
		scheduler.write(regs.SSP.LW - 4, addr_error_info(), size_type::WORD);

		// PUSH address HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, addr_error.address >> 16, size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		std::uint32_t addr = vector_address(curr_ex);
		scheduler.read(addr, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		scheduler.prefetch_two();
		
		return exec_state::done;
	}

	std::uint16_t addr_error_info() const
	{
		std::uint16_t status = regs.SIRD & ~0b11111; // undocumented behavior
		status |= addr_error.func_codes & 0x7; // first 3 bits

		if(addr_error.in)
			status |= 1 << 3; // 3rd bit
		
		if(addr_error.rw)
			status |= 1 << 4; // 4th bit

		return status;
	}

	/* Sequence of actions
	 * 1. Push PC
	 * 2. Push SR
	*/
	exec_state trap()
	{
		// TODO:
		if(trap_vector != 7)
			scheduler.wait(4 - 1);

		// PUSH PC LOW
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, regs.PC & 0xFFFF, size_type::WORD);

		// PUSH SR
		// note, for some reason we first push SR, then PC HIGH
		scheduler.write(regs.SSP.LW - 4, regs.SR, size_type::WORD);

		// update SR
		regs.flags.S = 1;
		regs.flags.TR = 0;

		// PUSH PC HIGH
		regs.SSP.LW -= 2;
		scheduler.write(regs.SSP.LW, regs.PC >> 16, size_type::WORD);
		regs.SSP.LW -= 2; // next word is already pushed on the stack

		std::uint32_t addr = trap_vector * 4;
		scheduler.read(addr, size_type::LONG, [this](std::uint32_t data, size_type)
		{
			regs.PC = data;
		});

		scheduler.prefetch_two();

		return exec_state::done;
	}

	static std::uint32_t vector_address(exception_type ex)
	{
		switch (ex)
		{
		case exception_type::bus_error:
			return 0x08;
		case exception_type::address_error:
			return 0x00C;
		case exception_type::trap:
			return 0x80;

		default: throw internal_error();
		}
	}

	void correct_pc()
	{
		bool is_move_long = (regs.SIRD >> 12) == 0b0010;
		bool is_move_word = (regs.SIRD >> 12) == 0b0011;
		bool write_op = !addr_error.rw;

		if((is_move_long || is_move_word) && write_op)
		{
			std::uint8_t mode = (regs.SIRD >> 6) & 0x7;
			if(mode == 0b100)
			{
				addr_error.PC += 2;
			}
		}
	}

	void abort_execution()
	{
		__abort_execution();
	}

private:
	m68k::exception_manager& exman;
	std::function<void()> __abort_execution;
	exception_type curr_ex;
	ex_state state;

	m68k::address_error addr_error;
	std::uint8_t trap_vector;
};

}

#endif // __M68K_EXCEPTION_UNIT_HPP__

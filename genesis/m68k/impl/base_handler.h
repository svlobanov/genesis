#ifndef __M68K_BASE_HANDLER_H__
#define __M68K_BASE_HANDLER_H__

#include <cstdint>

#include "bus_manager.hpp"
#include "prefetch_queue.hpp"
#include "m68k/cpu_registers.hpp"


namespace genesis::m68k
{

class base_handler
{
public:
	base_handler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq);
	virtual ~base_handler() { }

	void cycle();

	virtual void reset();

	// make final, not virtual
	virtual bool is_idle() const;

	// add protected method:
	// idle() --> to go to IDLE state
	// start_executing() --> to go to executing (!IDLE) state
	// bool has_work()????
	// on_idle
	// on_executing

protected:
	virtual void on_cycle() = 0;
	virtual void set_idle() = 0;

protected:
	/* interface for sub classes */

	// using on_read_complete = std::function<void(std::uint32_t /*addr*/, std::uint32_t /*data*/)>;

	void read_and_idle(std::uint32_t addr, std::uint8_t size, bus_manager::on_complete cb = nullptr);

	void read_byte(std::uint32_t addr, bus_manager::on_complete cb = nullptr);
	void read_word(std::uint32_t addr, bus_manager::on_complete cb = nullptr);
	void read_long(std::uint32_t addr, bus_manager::on_complete cb = nullptr);

	void read_imm(std::uint8_t size, bus_manager::on_complete cb = nullptr);

	// TODO: what about overloading?
	void write_byte(std::uint32_t addr, std::uint8_t data);
	void write_word(std::uint32_t addr, std::uint16_t data);
	void write_long(std::uint32_t addr, std::uint32_t data);

	void write_and_idle(std::uint32_t addr, std::uint32_t data, std::uint8_t size);
	void write_byte_and_idle(std::uint32_t addr, std::uint8_t data);
	void write_word_and_idle(std::uint32_t addr, std::uint16_t data);
	void write_long_and_idle(std::uint32_t addr, std::uint32_t data);

	void prefetch_one();
	void prefetch_two();
	void prefetch_irc();

	void prefetch_one_and_idle();
	void prefetch_two_and_idle();
	void prefetch_irc_and_idle();

	void wait(std::uint8_t cycles);
	void wait_and_idle(std::uint8_t cycles);
	void wait_after_idle(std::uint8_t cycles);

private:
	void call_on_cycle();

protected:
	m68k::cpu_registers& regs;
	m68k::bus_manager& busm;
	m68k::prefetch_queue& pq;

	std::uint32_t imm;
	std::uint32_t data;

private:
	std::uint8_t state;
	std::uint8_t cycles_to_wait;

	bool need_wait = false;
	std::uint8_t cycles_after_idle = 0;
};

}

#endif //__M68K_BASE_HANDLER_H__

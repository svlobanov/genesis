#ifndef __M68K_CPU_H__
#define __M68K_CPU_H__

#include "bus_access.h"
#include "cpu_bus.hpp"
#include "cpu_registers.hpp"
#include "impl/bus_manager.h"
#include "impl/bus_scheduler.h"
#include "impl/exception_manager.h"
#include "impl/exception_unit.hpp"
#include "impl/interrupt_riser.h"
#include "impl/trace_riser.hpp"
#include "interrupting_device.h"

#include <memory>


namespace genesis::m68k
{

class instruction_unit;

class cpu
{
public:
	cpu(std::shared_ptr<memory::addressable> external_memory,
		std::shared_ptr<interrupting_device> int_dev = std::make_shared<autovectored_interrupting_device>());
	~cpu();

	bool is_idle() const;

	void cycle();
	void reset();

	cpu_registers& registers()
	{
		return regs;
	}

	cpu_bus& bus()
	{
		return _bus;
	}

	m68k::bus_access& bus_access()
	{
		return bus_acs;
	}

	void set_interrupt(std::uint8_t priority);

protected:
	cpu_registers regs;
	cpu_bus _bus;
	std::shared_ptr<memory::addressable> external_memory;

	m68k::exception_manager exman;
	m68k::bus_manager busm;
	m68k::bus_access bus_acs;
	m68k::bus_scheduler scheduler;

	// TODO: do we really need pointers here?
	std::unique_ptr<m68k::instruction_unit> inst_unit;
	std::unique_ptr<m68k::exception_unit> excp_unit;
	std::unique_ptr<impl::trace_riser> tracer;
	std::unique_ptr<impl::interrupt_riser> m_int_riser;
};

} // namespace genesis::m68k

#endif // __M68K_CPU_H__

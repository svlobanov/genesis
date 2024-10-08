#include "cpu.h"

#include "impl/instruction_unit.hpp"

namespace genesis::m68k
{

cpu::cpu(std::shared_ptr<memory::addressable> external_memory, std::shared_ptr<interrupting_device> int_dev)
	: external_memory(external_memory), busm(_bus, regs, exman, external_memory, int_dev), bus_acs(busm),
	  scheduler(regs, busm)
{
	inst_unit = std::make_unique<m68k::instruction_unit>(regs, exman, _bus, scheduler);

	auto abort_execution = [this]() {
		inst_unit->reset();
		scheduler.reset();
		tracer->reset();
	};

	auto instruction_unit_is_idle = [this]() { return inst_unit->is_idle(); };

	excp_unit =
		std::make_unique<m68k::exception_unit>(regs, exman, _bus, scheduler, abort_execution, instruction_unit_is_idle);

	tracer = std::make_unique<impl::trace_riser>(regs, exman, instruction_unit_is_idle);

	m_int_riser = std::make_unique<impl::interrupt_riser>(regs, _bus, exman);

	reset();
}

cpu::~cpu()
{
}

void cpu::reset()
{
	// TODO: reset other units
	exman.rise(exception_type::reset);
}

void cpu::cycle()
{
	m_int_riser->cycle();

	// TODO: move to instruction unit
	// tracer->cycle();

	// only instruction or exception cycle
	bool exception_cycle = !excp_unit->is_idle();
	if(exception_cycle)
	{
		excp_unit->cycle();
	}
	else
	{
		inst_unit->cycle();
	}

	scheduler.cycle();
	busm.cycle();

	if(exception_cycle)
	{
		excp_unit->post_cycle();
	}
	else
	{
		inst_unit->post_cycle();
	}

	// tracer->post_cycle();
}

bool cpu::is_idle() const
{
	return busm.is_idle() && scheduler.is_idle() && inst_unit->is_idle() && excp_unit->is_idle();
}

void cpu::set_interrupt(std::uint8_t priority)
{
	// TODO: check if we support interrupts (int_dev is not null)

	if(priority > 7)
		throw std::invalid_argument("priority");

	_bus.interrupt_priority(priority);
}

} // namespace genesis::m68k

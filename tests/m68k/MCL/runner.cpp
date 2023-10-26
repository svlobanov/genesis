#include <gtest/gtest.h>

#include "mcl.h"
#include "string_utils.hpp"
#include "exception.hpp"
#include "helpers/random.h"

using namespace genesis;
using namespace genesis::test;
using namespace genesis::m68k;


TEST(M68K, MCL)
{
	test_cpu cpu;

	long cycles = 0;
	bool succeed = false;
	auto total_ns_time = measure_in_ns([&]() {
		succeed = run_mcl(cpu, [&cycles]() { ++cycles; });
	});

	auto ns_per_cycle = total_ns_time / cycles;
	std::cout << "NS per cycle for executing MCL test program: " << ns_per_cycle
		<< ", total cycles: " << cycles << std::endl;

	ASSERT_TRUE(succeed);
	ASSERT_NE(0, cycles);
	ASSERT_LT(ns_per_cycle, genesis::test::cycle_time_threshold_ns);
}

TEST(M68K, MCL_TAKE_BUS)
{
	/* Take control over the M68K bus multiple times during MCL program execution */

	// it takes at least 4 cycles to execute single bus operatoin,
	// so make this constant not divisible by 4
	// to request bus after the bus cycle and in the middle of bus cycle
	const long request_bus_cycles_threshold = 1001;

	const long request_bus_cycles_duration = 41;

	test_cpu cpu;

	auto& busm = cpu.bus_access();

	long cycles = 0;
	long cycles_after_bus_requested = 0;
	long cycles_after_bus_granted = 0;
	long cycles_after_release_requested = 0;

	bool bus_requested = false;
	bool release_requested = false;

	bool succeed = run_mcl(cpu, [&]()
	{
		++cycles;

		if((cycles % request_bus_cycles_threshold) == 0)
		{
			ASSERT_FALSE(busm.bus_granted());
			ASSERT_FALSE(bus_requested);
			ASSERT_FALSE(release_requested);

			ASSERT_EQ(cycles_after_bus_requested, 0);
			ASSERT_EQ(cycles_after_bus_granted, 0);
			ASSERT_EQ(cycles_after_release_requested, 0);

			busm.request_bus();
			bus_requested = true;

			return;
		}

		if(bus_requested)
		{
			++cycles_after_bus_requested;

			// it may take up to 10 cycles to execute bus cycle (TAS instruction)
			// it can take another cycle to grant access
			// and add a few cycles just in case
			ASSERT_LE(cycles_after_bus_requested, 15);

			if(busm.bus_granted())
			{
				bus_requested = false;
				cycles_after_bus_requested = 0;
			}
		}

		if(release_requested)
		{
			++cycles_after_release_requested;

			// we do not use bus in this test, so it should take a few cycles to release the bus
			ASSERT_LE(cycles_after_release_requested, 4);

			if(!busm.bus_granted())
			{
				release_requested = false;
				cycles_after_release_requested = 0;
			}
		}

		if(busm.bus_granted())
		{
			++cycles_after_bus_granted;

			// we do not use bus when we acquire access, so bus manager must be always idle
			ASSERT_TRUE(busm.is_idle());

			if(cycles_after_bus_granted == request_bus_cycles_duration)
			{
				busm.release_bus();
				release_requested = true;
			}
		}
		else
		{
			cycles_after_bus_granted = 0;
		}
	});

	ASSERT_TRUE(succeed);
}

TEST(M68K, MCL_TAKE_BUS_TO_READ_WRITE)
{
	/* Take control over the M68K bus and pefrom read/write bus cycles during MCL program execution */

	const long request_bus_cycles_threshold = 1001;

	enum class test_state { run, starting, read, reading, write, writing };

	test_state state = test_state::run;
	test_state old_state = state;

	test_cpu cpu;

	auto& busm = cpu.bus_access();
	auto& mem = cpu.memory();

	long cycles = 0;
	long cycles_in_the_same_state = 0;
	std::uint32_t address = 0;
	std::uint8_t old_data = 0;
	std::uint8_t new_data = 0;

	bool succeed = run_mcl(cpu, [&]()
	{
		switch (state)
		{
		case test_state::run:
		{
			++cycles;

			if((cycles % request_bus_cycles_threshold) == 0)
			{
				ASSERT_FALSE(busm.bus_granted());

				busm.request_bus();
				state = test_state::starting;
			}
		}
		break;

		case test_state::starting:
		{
			if(busm.is_idle() && busm.bus_granted())
			{
				state = test_state::read;
			}
		}
		break;

		case test_state::read:
		{
			ASSERT_TRUE(busm.bus_granted());
			ASSERT_TRUE(busm.is_idle());

			// start read
			address = test::random::next<std::uint32_t>() % (mem.max_address() + 1);
			busm.init_read_byte(address, m68k::addr_space::PROGRAM);
			state = test_state::reading;
		}
		break;

		case test_state::reading:
		{
			ASSERT_TRUE(busm.bus_granted());

			if(busm.is_idle())
			{
				// make sure read operation still works
				std::uint8_t read_data = busm.latched_byte();
				std::uint8_t expected_data = mem.read<std::uint8_t>(address);
				ASSERT_EQ(expected_data, read_data);

				state = test_state::write;
			}
		}
		break;

		case test_state::write:
		{
			ASSERT_TRUE(busm.bus_granted());
			ASSERT_TRUE(busm.is_idle());

			// start write
			address = test::random::next<std::uint32_t>() % (mem.max_address() + 1);
			old_data = mem.read<std::uint8_t>(address);
			new_data = test::random::next<std::uint8_t>();

			busm.init_write(address, new_data);
			state = test_state::writing;
		}
		break;

		case test_state::writing:
		{
			if(busm.is_idle())
			{
				// make sure data has been written
				std::uint8_t actual_data = mem.read<std::uint8_t>(address);
				ASSERT_EQ(new_data, actual_data);

				// set data back
				mem.write(address, old_data);
				busm.release_bus();

				state = test_state::run;
			}
		}
		break;
		
		default:
			throw internal_error();
		}

		// make sure we don't state in the same state for too long
		if(state != test_state::run)
		{
			if(state == old_state)
			{
				++cycles_in_the_same_state;
			}
			else
			{
				cycles_in_the_same_state = 0;
			}

			old_state = state;

			ASSERT_NE(cycles_in_the_same_state, 15);
		}
	});

	ASSERT_TRUE(succeed);
}

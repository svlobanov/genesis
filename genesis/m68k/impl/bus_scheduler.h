#ifndef __M68K_BUS_SCHEDULER_H__
#define __M68K_BUS_SCHEDULER_H__

#include "bus_manager.h"
#include "m68k/cpu_registers.hpp"
#include "prefetch_queue.hpp"

#include <functional>
#include <optional>
#include <queue>
#include <variant>


namespace genesis::m68k
{

enum class order
{
	lsw_first, // least significant word first
	msw_first, // most significant word first
};

enum class read_imm_flags
{
	// specify whether need to prefetch IRC or not after read
	do_prefetch,
	no_prefetch,
};

// TODO: maybe back to scheduler?
class bus_scheduler
{
private:
	// all callbacks are restricted in size to the size of the pointer
	// this is required for std::function small-size optimizations
	// (though it's not guaranteed by standard, so we purely rely on implementation)
	constexpr const static std::size_t max_callable_size = sizeof(void*);

public:
	using on_read_complete = std::function<void(std::uint32_t /*data*/, size_type)>;

public:
	bus_scheduler(m68k::cpu_registers& regs, m68k::bus_manager& busm);
	~bus_scheduler() = default;

	void cycle();
	bool is_idle() const;
	void reset();

	template <class Callable>
	void read(std::uint32_t addr, size_type size, Callable on_complete)
	{
		read(addr, size, addr_space::DATA, on_complete);
	}

	template <class Callable>
	void read(std::uint32_t addr, size_type size, addr_space space, Callable on_complete)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_impl(addr, size, space, on_complete);
	}

	template <class Callable = std::nullptr_t>
	void read_imm(size_type size, Callable on_complete = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_imm_impl(size, on_complete);
	}

	template <class Callable = std::nullptr_t>
	void read_imm(size_type size, read_imm_flags flags, Callable on_complete = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_imm_impl(size, on_complete, flags);
	}

	void write(std::uint32_t addr, std::uint32_t data, size_type size, order order = order::lsw_first);

	template <class Callable>
	void read_modify_write(std::uint32_t addr, Callable modify)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_modify_write_impl(addr, modify);
	}

	template <class Callable>
	void int_ack(std::uint8_t ipl, Callable on_complete)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		int_ack_impl(ipl, on_complete);
	}

	void prefetch_ird();
	void prefetch_irc();
	void prefetch_one();
	void prefetch_two();

	void wait(int cycles);

	template <class Callable>
	void call(Callable cb)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		call_impl(cb);
	}

	void inc_addr_reg(int reg, size_type size);
	void dec_addr_reg(int reg, size_type size);

	void push(std::uint32_t data, size_type size, order order = order::msw_first);

private:
	enum class op_type
	{
		READ,
		READ_IMM,
		WRITE,
		RMW, // Read Modify Write
		INT_ACK,
		PREFETCH_IRD,
		PREFETCH_IRC,
		PREFETCH_ONE,
		WAIT,
		CALL,
		INC_ADDR,
		DEC_ADDR,
		PUSH,
	};

	struct read_operation
	{
		std::uint32_t addr;
		size_type size;
		addr_space space;
		on_read_complete on_complete;
	};

	struct read_imm_operation
	{
		size_type size;
		on_read_complete on_complete;
		read_imm_flags flags;
	};

	struct write_operation
	{
		std::uint32_t addr;
		std::uint32_t data;
		size_type size;
	};

	using on_modify = std::function<std::uint8_t(std::uint8_t)>;
	struct rmw_operation
	{
		std::uint32_t addr;
		on_modify modify;
	};

	using int_ack_complete = std::function<void(std::uint8_t /* vector number */)>;
	struct int_ack_operation
	{
		std::uint8_t ipl; // interrupt priority level
		int_ack_complete on_complete;
	};

	struct wait_operation
	{
		int cycles;
	};

	using callback = std::function<void()>;
	struct call_operation
	{
		callback cb;
	};

	struct register_operation
	{
		int reg;
		size_type size;
	};

	struct push_operation
	{
		std::uint32_t data;
		size_type size;
		int offset = 0;
	};

	struct operation
	{
		op_type type;
		std::variant<read_operation, read_imm_operation, rmw_operation, int_ack_operation, write_operation,
					 wait_operation, call_operation, register_operation, push_operation>
			op = {};
	};

private:
	void read_impl(std::uint32_t addr, size_type size, addr_space space, on_read_complete on_complete);
	void read_imm_impl(size_type size, on_read_complete on_complete,
					   read_imm_flags flags = read_imm_flags::do_prefetch);
	void read_modify_write_impl(std::uint32_t addr, on_modify modify);
	void int_ack_impl(std::uint8_t ipl, int_ack_complete);
	void call_impl(callback);

	void latch_data(size_type size);
	void on_read_finished();
	void on_read_imm_finished();
	void on_int_ack_finished();

	bool current_op_is_over() const;
	void start_operation(operation&);
	void run_cycless_operations();

	// TODO: add bus_read / bus_write operations to hide all busm calls
	bool next_bus_operation() const;
	bool can_use_bus() const;

private:
	m68k::cpu_registers& regs;
	m68k::bus_manager& busm;
	m68k::prefetch_queue pq;

	// TODO: replace to const size queue
	std::queue<operation> queue;
	std::optional<operation> current_op;
	std::uint32_t data = 0;
	int curr_wait_cycles = 0;
};


}; // namespace genesis::m68k

#endif // __M68K_BUS_SCHEDULER_H__

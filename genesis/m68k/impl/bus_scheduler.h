#ifndef __M68K_BUS_SCHEDULER_H__
#define __M68K_BUS_SCHEDULER_H__

#include <queue>
#include <variant>
#include <optional>
#include <functional>

#include "m68k/cpu_registers.hpp"
#include "bus_manager.hpp"
#include "prefetch_queue.hpp"


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

	template<class Callable>
	void read(std::uint32_t addr, size_type size, Callable on_complete)
	{
		read(addr, size, addr_space::DATA, on_complete);
	}

	template<class Callable>
	void read(std::uint32_t addr, size_type size, addr_space space, Callable on_complete)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_impl(addr, size, space, on_complete);
	}

	template<class Callable = std::nullptr_t>
	void read_imm(size_type size, Callable on_complete = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_imm_impl(size, on_complete);
	}

	template<class Callable = std::nullptr_t>
	void read_imm(size_type size, read_imm_flags flags, Callable on_complete = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		read_imm_impl(size, on_complete, flags);
	}

	void write(std::uint32_t addr, std::uint32_t data, size_type size, order order = order::lsw_first);

	void prefetch_ird();
	void prefetch_irc();
	void prefetch_one();
	void prefetch_two();

	void wait(std::uint_fast8_t cycles);

	template<class Callable>
	void call(Callable cb)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		call_impl(cb);
	}

	void inc_addr_reg(std::uint_fast8_t reg, size_type size);
	void dec_addr_reg(std::uint_fast8_t reg, size_type size);

	void push(std::uint32_t data, size_type size, order order = order::msw_first);

public:
	enum class op_type
	{
		READ,
		READ_IMM,
		WRITE,
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

	struct wait_operation
	{
		std::uint_fast8_t cycles;
	};

	using callback = std::function<void()>;
	struct call_operation
	{
		callback cb;
	};

	struct register_operation
	{
		std::uint_fast8_t reg;
		size_type size;
	};

	struct push_operation
	{
		std::uint32_t data;
		size_type size;
		std::int_fast8_t offset = 0;
	};

	struct operation
	{
		op_type type;
		std::variant<read_operation, read_imm_operation,
			write_operation, wait_operation, call_operation,
			register_operation, push_operation> op;
	};

private:
	void read_impl(std::uint32_t addr, size_type size, addr_space space, on_read_complete on_complete);
	void read_imm_impl(size_type size, on_read_complete on_complete, read_imm_flags flags = read_imm_flags::do_prefetch);
	void call_impl(callback);

	void latch_data(size_type size);
	void on_read_finished();
	void on_read_imm_finished();

	bool current_op_is_over() const;
	void start_operation(operation&);
	void run_cycless_operations();

	// TODO: add bus_read / bus_write operations to hide all busm calls

private:
	m68k::cpu_registers& regs;
	m68k::bus_manager& busm;
	m68k::prefetch_queue pq;

	// TODO: replace to stack-allocated queue
	std::queue<operation> queue;
	std::optional<operation> current_op;
	std::uint32_t data = 0;
	std::uint_fast8_t curr_wait_cycles = 0;
};


};

#endif // __M68K_BUS_SCHEDULER_H__

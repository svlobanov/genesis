#ifndef __HELPER_HPP__
#define __HELPER_HPP__

#include <filesystem>
#include <gtest/gtest.h>


[[maybe_unused]] static std::filesystem::path get_exec_path()
{
	// I'm sorry for that...
	std::filesystem::path exec_path = testing::internal::GetArgvs()[0];
	exec_path.remove_filename();

	return std::filesystem::absolute(exec_path);
}

#endif // __HELPER_HPP__

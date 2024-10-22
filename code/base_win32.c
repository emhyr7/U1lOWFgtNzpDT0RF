#include "base_win32.h"

struct
{
	HINSTANCE    instance;
	STARTUPINFOW startup_info;

	const utf16 *command_line;

	uintl performance_frequency;
} win32;

/* platform */

void initialize_platform(void)
{
	win32.instance = GetModuleHandle(0);
	GetStartupInfoW(&win32.startup_info);

	win32.command_line = GetCommandLineW();
	base.command_line_size = make_utf8_text_from_utf16(0, win32.command_line);
	base.command_line = push(base.command_line_size + 1, universal_alignment, &base.persistent_allocator);
	make_utf8_text_from_utf16(base.command_line, win32.command_line);

	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	win32.performance_frequency = frequency.QuadPart;
}

/* time */

inline uintl get_time(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * 1e9 / win32.performance_frequency;
}

/* memory */

inline void *allocate(uint size)
{
	void *memory = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	ASSERT(memory);
	return memory;
}

inline void deallocate(void *memory, uint size)
{
	OMIT(size);
	VirtualFree(memory, 0, MEM_RELEASE);
}

/* files */

inline uint get_current_directory_path(utf8 *path)
{
	utf16 path_buffer[maximum_size_of_path];
	uint size = GetCurrentDirectoryW(sizeof(path_buffer), path_buffer);
	make_utf8_text_from_utf16(path, path_buffer);
	return size;
}

inline handle create_file(const utf8 *path)
{
	handle file = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	ASSERT(file != INVALID_HANDLE_VALUE);
	return file;
}

inline handle open_file(const utf8 *path)
{
	handle file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ASSERT(file != INVALID_HANDLE_VALUE);
	return file;
}

inline uintl get_size_of_file(handle file)
{
	LARGE_INTEGER file_size;
	ASSERT(GetFileSizeEx(file, &file_size));
	return file_size.QuadPart;
}

inline uint read_from_file(void *buffer, uint size, handle file)
{
	DWORD bytes_read_count;
	ASSERT(ReadFile(file, buffer, size, &bytes_read_count, 0));
	return bytes_read_count;
}

inline void close_file(handle file)
{
	CloseHandle(file);
}

#include "code.h"

#include "code_parser.c"
//#include "code_checker.c"

struct
{
	HINSTANCE    instance;
	STARTUPINFOW startup_info;

	const utf16 *command_line;

	uintl performance_frequency;
} win32;

int main(int arguments_count, char *arguments[])
{
	setlocale(LC_CTYPE, "");
	/* initialize platform-specific stuff */
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

	context.failure_landing = &context.default_failure_landing;
	if (SET_LANDING(context.default_failure_landing))
	{
		UNIMPLEMENTED();
	}
	
	program program;

	if (arguments_count <= 1)
	{
		REPORT_FAILURE("A source path wasn't given.");
		return -1;
	}

	parser parser;
	parser_parse(arguments[1], &program, &parser);
}

/* math */

inline address get_backward_alignment(address address, uintb alignment)
{
	if (alignment == 1) alignment = 0;
	return alignment ? address & (alignment - 1) : 0;
}

inline address get_forward_alignment(address address, uintb alignment)
{
	uintl remainder = get_backward_alignment(address, alignment);
	return remainder ? alignment - remainder : 0;
}

inline address align_forward(address address, uintb alignment)
{
	return address + get_forward_alignment(address, alignment);
}

inline address align_backward(address address, uintb alignment)
{
	return address - get_backward_alignment(address, alignment);
}

inline void copy(void *left, const void *right, uint size)
{
	__builtin_memcpy(left, right, size);
}

inline void move(void *left, const void *right, uint size)
{
	__builtin_memmove(left, right, size);
}

inline void fill(void *left, uint size, byte value)
{
	__builtin_memset(left, value, size);
}

inline void zero(void *left, uint size)
{
	fill(left, size, 0);
}

inline sints compare(const void *left, const void *right, uint size)
{
	return memcmp(left, right, size);
}

inline uintb clz(uintl value)
{
#if defined(CODE_ON_PLATFORM_WIN32)
  unsigned long index;
  if (!_BitScanReverse64(&index, value))
    return sizeof(value) * byte_bits_count;
  return sizeof(value) * byte_bits_count - index - 1;
#elif defined(ON_LINUX)
  return __builtin_clz(value);
#endif
}

inline uintb ctz(uintl value)
{
#if defined(CODE_ON_PLATFORM_WIN32)
  unsigned long index;
  if (!_BitScanForward64(&index, value))
    return sizeof(value) * byte_bits_count;
  return index;
#elif defined(ON_LINUX)
  return __builtin_ctz(value);
#endif
}

/*
ALGORITHM

  twice the byte granularity of bits are buffered as a bitmask that's
  iteratively compared to the input bitset by shifting the bitmask.

  the amount of shifts for the bitmask is calculated by:
  1) the amount of trailing zeros in the bitset; and
  2) the amount of leading zeros in the bitmask.
  in other words, we skip ranges of zeros, and try to skip to a leading range
  of ones in the bitmask. (we "try" since there may not be any leading range of
  ones in the bitmask, at which case the bitmask is completely skipped).

  when the total amount of shifts is clamped and reset at the the byte
  granularity, we extract the next byte from the bitset into the bitmask.
*/
sintl index_bit_range(uintb range_size, bit of_zeros, const uint *bytes, uint bytes_count)
{
  sintl range_index = -1;
  uintb byte_granularity = sizeof(*bytes) * byte_bits_count; 

  /* i know... it's not ideal to limit the range */
  assert(range_size <= byte_granularity * 2);

  uintl mask = (1 << range_size) - 1;
  uintb shift_count = 0;
  uint byte_index = 0;
  for (;;)
  {
    if (byte_index == bytes_count) break;

    /* buffer bits as a bitmask */
    uintl buffer = (uintl)0 | (uintl)bytes[byte_index];
    if (byte_index != bytes_count - 1) buffer |= ((uintl)bytes[byte_index + 1] << byte_granularity);

    /* flip the bits because the user wants to search for a range of zeros
       instead of a range of ones */
    if (of_zeros) buffer = ~buffer;

    /* skip the trailing zeros */
    shift_count += ctz(buffer >> shift_count);

    /* try to extract the next byte */
    if (shift_count >= byte_granularity)
    {
      ++byte_index;
      shift_count = 0;
      continue;
    }

    /* skip the compare the bitmask */
    uintl shifted_mask = mask << shift_count;
    uintl masked = buffer & shifted_mask;
    if (masked == shifted_mask)
    {
      range_index = byte_index * byte_granularity + shift_count;
      break;
    }

    /* try to skip to the leading range of ones */
    uintb leading_zeros_count = clz(~masked << (byte_granularity * 2 - range_size));
    shift_count += range_size - leading_zeros_count;
  }
  return range_index;
}

/* text */

sintb decode_utf8(utf32 *left, const utf8 *right)
{
	const byte utf8_classes[32] =
	{
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		0, 0, 0, 0, 0, 0, 0, 0,
		2, 2, 2, 2,
		3, 3,
		4,
		5,
	};

	byte bytes[4]  = { right[0] };
	byte increment = utf8_classes[bytes[0] >> 3];
	switch (increment)
	{
	case 1:
		*left = bytes[0];
		break;
	case 2:
		bytes[1] = right[1];
		if (!utf8_classes[bytes[1] >> 3])
		{
			*left = ((bytes[0] & lmask5) << 6)
				  | ((bytes[1] & lmask6) << 0);
		}
		else increment = -increment;
		break;
	case 3:
		bytes[1] = right[1];
		bytes[2] = right[2];
		if (!utf8_classes[bytes[1] >> 3] &&
			!utf8_classes[bytes[2] >> 3])
		{
			*left = ((bytes[0] & lmask4) << 12)
				  | ((bytes[1] & lmask6) <<  6)
				  | ((bytes[2] & lmask6) <<  0);
		}
		else increment = -increment;
		break;
	case 4:
		bytes[1] = right[1];
		bytes[2] = right[2];
		bytes[3] = right[3];
		if (!utf8_classes[bytes[1] >> 3] &&
			!utf8_classes[bytes[2] >> 3] &&
			!utf8_classes[bytes[3] >> 3])
		{
			*left = ((bytes[0] & lmask3) << 18)
				  | ((bytes[1] & lmask6) << 12)
				  | ((bytes[2] & lmask6) <<  6)
				  | ((bytes[3] & lmask6) <<  0);
		}
		else increment = -increment;
		break;
	default:
		increment = -1;
		break;
	}
	return increment;
}

sintb decode_utf16(utf32 *left, const utf16 *right)
{
	*left = *right;
	uintb increment = 1;
	if (0xd800 <= right[0] && right[0] < 0xdc00)
	{
		if (0xdc00 <= right[1] && right[1] < 0xe000)
		{
			*left = ((right[0] - 0xd800) << 10) | (right[1] - 0xdc00) + 0x10000;
			increment = 2;
		}
		else increment = -1;
	}
	return increment;
}

sintb encode_utf8(utf8 *left, utf32 right)
{
	byte buffer[4];
	uint increment = 0;
	if (right <= 0x7f)
	{
		buffer[0] = right;
		increment = 1;
	}
	else if (right <= 0x7ff)
	{
		buffer[0] = (lmask2 << 6) | ((right >> 6) & lmask5);
		buffer[1] = (bit8       ) | ((right >> 0) & lmask6);
		increment = 2;
	}
	else if (right <= 0xffff)
	{
		buffer[0] = (lmask3 << 5) | ((right >> 12) & lmask4);
		buffer[1] = (bit8       ) | ((right >>  6) & lmask6);
		buffer[2] = (bit8       ) | ((right >>  0) & lmask6);
		increment = 3;
	}
	else if (right <= 0x10ffff)
	{
		buffer[0] = (lmask4 << 4) | ((right >> 18) & lmask3);
		buffer[1] = (bit8       ) | ((right >> 12) & lmask6);
		buffer[2] = (bit8       ) | ((right >>  6) & lmask6);
		buffer[3] = (bit8       ) | ((right >>  0) & lmask6);
		increment = 4;
	}
	else increment = -1;
	if (left && increment > 0) COPY(left, buffer, increment);
	return increment;
}

sintb encode_utf16(utf16 *left, utf32 right)
{
	byte buffer[2];
	sintb increment = 0;
	if (right < 0x10000)
	{
		buffer[0] = right;
		increment = 1;
	}
	else if (right != uint32_maximum)
	{
		uint v = right - 0x10000;
		buffer[0] = 0xd800 + (v >> 10);
		buffer[1] = 0xdc00 + (v & lmask10);
		increment = 2;
	}
	else increment = -1;
	if (left && increment > 0) COPY(left, buffer, increment);
	return increment;
}

sintl make_utf8_text_from_utf16(utf8 *left, const utf16 *right)
{
	utf8 *left_caret = left;
	const utf16 *right_caret = right;
	while (*right_caret)
	{
		utf32 utf32;
		sintb increment = decode_utf16(&utf32, right_caret);
		if (increment < 0) return increment;
		right_caret += increment;
		left_caret += encode_utf8(left ? left_caret : 0, utf32);
	}
	if (left) *left_caret = 0;
	return left_caret - left;
}

sintl make_utf16_text_from_utf8(utf16 *left, const utf8 *right)
{
	utf16 *left_caret = left;
	const utf8 *right_caret = right;
	while (*right_caret)
	{
		utf32 utf32;
		sintb increment = decode_utf8(&utf32, right_caret);
		if (increment < 0) return increment;
		right_caret += increment;
		left_caret += encode_utf16(left ? left_caret : 0, utf32);
	}
	if (left) *left_caret = 0;
	return left_caret - left;
}

inline uint get_size_of_utf8_text(const utf8 *text)
{
	return __builtin_strlen(text);
}

inline uint get_size_of_utf16_text(const utf16 *text)
{
	uint size = 0;
	while (*text++) size++;
	return size;
}

inline sint compare_text(const utf8 *left, const utf8 *right)
{
	return __builtin_strcmp(left, right);
}

inline sint compare_sized_text(const utf8 *left, const utf8 *right, uint size)
{
	return __builtin_strncmp(left, right, size);
}

inline void copy_text(utf8 *left, const utf8 *right)
{
	__builtin_strcpy(left, right);
}

inline void copy_sized_text(utf8 *left, const utf8 *right, uint size)
{
	__builtin_strncpy(left, right, size);
}

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

inline sintl format_text_v(utf8 *buffer, uint size, const utf8 *format, vargs vargs)
{
	return __builtin_vsnprintf(buffer, size, format, vargs);
}

inline sintl format_text(utf8 *buffer, uint size, const utf8 *format, ...)
{
	vargs vargs;
	GET_VARGS(vargs, format);
	sintl result = format_text_v(buffer, size, format, vargs);
	END_VARGS(vargs);
	return result;
}

inline void jump(landing landing, int status)
{
	longjmp(landing, status);
}

void _report(const char *file, uint line, severity severity, const char *message, ...)
{
	/* printf("[%s] %s:%u: ", severity_representations[severity], file, line); */
	printf("[%s] ", severities_representations[severity]);

	vargs vargs;
	GET_VARGS(vargs, message);
	vprintf(message, vargs);
	END_VARGS(vargs);
}

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

void *push(uint size, uint alignment, regional_allocator *allocator)
{
	if (!allocator->active_region) allocator->active_region = allocator->first_region;

	uint forward_alignment;
	bit  would_overflow = 1;
	if (allocator->active_region)
	{
		for (;;)
		{
			forward_alignment = get_forward_alignment((uintl)allocator->active_region->view + allocator->active_region->mass, alignment);
			would_overflow = allocator->active_region->mass + forward_alignment + size > allocator->active_region->size;
			if (!would_overflow) break;
			if (!allocator->active_region->next) break;
			allocator->active_region = allocator->active_region->next;
		}
	}

	if (would_overflow)
	{
		forward_alignment = 0;
		if (!allocator->minimum_region_size) allocator->minimum_region_size = default_minimum_region_size_of_regional_allocator;
		uint region_size = MAXIMUM(size, allocator->minimum_region_size);
		uint allocation_size = sizeof(region) + region_size;
		region *new_region = allocate(allocation_size);
		new_region->prior = allocator->active_region;
		if (allocator->active_region) allocator->active_region->next = new_region;
		new_region->mass = 0;
		new_region->size = region_size;
		new_region->view = new_region->data;
		new_region->next = 0;
		allocator->active_region = new_region;
		if (!allocator->first_region) allocator->first_region = allocator->active_region;
	}

	allocator->active_region->mass += forward_alignment;
	void *memory = allocator->active_region->view + allocator->active_region->mass;
	allocator->active_region->mass += size;
	fill(memory, size, 0);
	return memory;
}

thread_local struct context context;

struct base base;

inline uintl get_time(void)
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart * 1e9 / win32.performance_frequency;
}

inline uint get_current_directory_path(utf8 *path)
{
	utf16 path_buffer[maximum_size_of_path];
	uint size = GetCurrentDirectoryW(sizeof(path_buffer), path_buffer);
	make_utf8_text_from_utf16(path, path_buffer);
	return size;
}

inline file_handle create_file(const utf8 *path)
{
	file_handle result = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	ASSERT(result != INVALID_HANDLE_VALUE);
	return result;
}

inline file_handle open_file(const utf8 *path)
{
	file_handle result = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	ASSERT(result != INVALID_HANDLE_VALUE);
	return result;
}

inline uintl get_size_of_file(file_handle handle)
{
	LARGE_INTEGER file_size;
	ASSERT(GetFileSizeEx(handle, &file_size));
	return file_size.QuadPart;
}

inline uint read_from_file(void *buffer, uint size, file_handle handle)
{
	DWORD bytes_read_count;
	ASSERT(ReadFile(handle, buffer, size, &bytes_read_count, 0));
	return bytes_read_count;
}

inline void close_file(file_handle handle)
{
	CloseHandle(handle);
}

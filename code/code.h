#if !defined(CODE_H)
#define CODE_H

#if !defined(NDEBUG)
	#define CODE_DEBUGGING
#endif

#if defined(_WIN32)
	#define CODE_ON_PLATFORM_WIN32
#elif defined(__linux__)
	#define CODE_ON_PLATFORM_LINUX
#endif

#define UNICODE
#define _UNICODE
#include <Windows.h>

#include <assert.h>
#include <locale.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>
#include <wchar.h>
#include <stddef.h>
#include <setjmp.h>

#define UNREACHABLE() __builtin_unreachable()

#define EXPECT(x, v)  __builtin_expect((x), (v))
#define LIKELY(x)   EXPECT(x, 1)
#define UNLIKELY(x) EXPECT(x, 0)

#define ASSERT(...)     assert(__VA_ARGS__)
#define UNIMPLEMENTED() ({ASSERT(!"UNIMPLEMENTED"); UNREACHABLE(); })

#define OMIT(x) (void)x

#define COUNT(x)     (sizeof(x) / sizeof(x[0]))
#define OFFSET(t, x) (uint)(&(((t*)0)->x) - 0)

#define KIB(x) ((x) << 10)
#define MIB(x) ((x) << 20)
#define GIG(x) (((uint64)x) << 30)

typedef va_list vargs;
#define GET_VARGS(...)  va_start(__VA_ARGS__) 
#define END_VARGS(...)  va_end(__VA_ARGS__) 
#define COPY_VARGS(...) va_copy(__VA_ARGS__)
#define GET_VARG(...)   va_get(__VA_ARGS__)

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t  sint8;
typedef int16_t sint16;
typedef int32_t sint32;
typedef int64_t sint64;

typedef float  float32;
typedef double float64;

typedef char    utf8;
typedef wchar_t utf16;
typedef uint32  utf32;

#define UINT_MAXIMUM(type) ((type)~0)

constexpr uint8  uint8_maximum  = UINT_MAXIMUM(uint8);
constexpr uint16 uint16_maximum = UINT_MAXIMUM(uint16);
constexpr uint32 uint32_maximum = UINT_MAXIMUM(uint32);
constexpr uint64 uint64_maximum = UINT_MAXIMUM(uint64);

constexpr uint8  uint8_minimum  = 0;
constexpr uint16 uint16_minimum = 0;
constexpr uint32 uint32_minimum = 0;
constexpr uint64 uint64_minimum = 0;

#define SINT_MAXIMUM(type) (type)((1ull << (sizeof(type) * 8 - 1)) - 1)

constexpr sint8  sint8_maximum  = SINT_MAXIMUM(sint8);
constexpr sint16 sint16_maximum = SINT_MAXIMUM(sint16);
constexpr sint32 sint32_maximum = SINT_MAXIMUM(sint32);
constexpr sint64 sint64_maximum = SINT_MAXIMUM(sint64);

constexpr sint8  sint8_minimum  = -1 - sint8_maximum;
constexpr sint16 sint16_minimum = -1 - sint16_maximum;
constexpr sint32 sint32_minimum = -1 - sint32_maximum;
constexpr sint64 sint64_minimum = -1 - sint64_maximum;

typedef uint8  uintb;
typedef uint16 uints;
typedef uint32 uint;
typedef uint64 uintl;

typedef sint8  sintb;
typedef sint16 sints;
typedef sint32 sint;
typedef sint64 sintl;

typedef uintb byte;
typedef byte  bit; // the possible values should _only_ be 0 or 1

constexpr uint byte_bits_count = 8;

typedef uint8  bits8;
typedef uint16 bits16;
typedef uint32 bits32;
typedef uint64 bits64;

typedef bits64 address;

#define BIT(x) (1ull << (x - 1))

constexpr bits64 bit1  = BIT(1);
constexpr bits64 bit2  = BIT(2);
constexpr bits64 bit3  = BIT(3);
constexpr bits64 bit4  = BIT(4);
constexpr bits64 bit5  = BIT(5);
constexpr bits64 bit6  = BIT(6);
constexpr bits64 bit7  = BIT(7);
constexpr bits64 bit8  = BIT(8);
constexpr bits64 bit9  = BIT(9);
constexpr bits64 bit10 = BIT(10);
constexpr bits64 bit11 = BIT(11);
constexpr bits64 bit12 = BIT(12);
constexpr bits64 bit13 = BIT(13);
constexpr bits64 bit14 = BIT(14);
constexpr bits64 bit15 = BIT(15);
constexpr bits64 bit16 = BIT(16);
constexpr bits64 bit17 = BIT(17);
constexpr bits64 bit18 = BIT(18);
constexpr bits64 bit19 = BIT(19);
constexpr bits64 bit20 = BIT(20);
constexpr bits64 bit21 = BIT(21);
constexpr bits64 bit22 = BIT(22);
constexpr bits64 bit23 = BIT(23);
constexpr bits64 bit24 = BIT(24);
constexpr bits64 bit25 = BIT(25);
constexpr bits64 bit26 = BIT(26);
constexpr bits64 bit27 = BIT(27);
constexpr bits64 bit28 = BIT(28);
constexpr bits64 bit29 = BIT(29);
constexpr bits64 bit30 = BIT(30);
constexpr bits64 bit31 = BIT(31);
constexpr bits64 bit32 = BIT(32);

#define LMASK(x) (BIT(x) - 1)

constexpr bits64 lmask1  = LMASK(1);
constexpr bits64 lmask2  = LMASK(2);
constexpr bits64 lmask3  = LMASK(3);
constexpr bits64 lmask4  = LMASK(4);
constexpr bits64 lmask5  = LMASK(5);
constexpr bits64 lmask6  = LMASK(6);
constexpr bits64 lmask7  = LMASK(7);
constexpr bits64 lmask8  = LMASK(8);
constexpr bits64 lmask9  = LMASK(9);
constexpr bits64 lmask10 = LMASK(10);
constexpr bits64 lmask11 = LMASK(11);
constexpr bits64 lmask12 = LMASK(12);
constexpr bits64 lmask13 = LMASK(13);
constexpr bits64 lmask14 = LMASK(14);
constexpr bits64 lmask15 = LMASK(15);
constexpr bits64 lmask16 = LMASK(16);
constexpr bits64 lmask17 = LMASK(17);
constexpr bits64 lmask18 = LMASK(18);
constexpr bits64 lmask19 = LMASK(19);
constexpr bits64 lmask20 = LMASK(20);
constexpr bits64 lmask21 = LMASK(21);
constexpr bits64 lmask22 = LMASK(22);
constexpr bits64 lmask23 = LMASK(23);
constexpr bits64 lmask24 = LMASK(24);
constexpr bits64 lmask25 = LMASK(25);
constexpr bits64 lmask26 = LMASK(26);
constexpr bits64 lmask27 = LMASK(27);
constexpr bits64 lmask28 = LMASK(28);
constexpr bits64 lmask29 = LMASK(29);
constexpr bits64 lmask30 = LMASK(30);
constexpr bits64 lmask31 = LMASK(31);
constexpr bits64 lmask32 = LMASK(32);

address get_backward_alignment(address address, uintb alignment);
address get_forward_alignment (address address, uintb alignment);

address align_forward (address address, uintb alignment);
address align_backward(address address, uintb alignment);

#define MINIMUM(a, b) ((a) < (b) ? (a) : (b))
#define MAXIMUM(a, b) ((a) > (b) ? (a) : (b))

void copy(void *left, const void *right, uint size);
void move(void *left, const void *right, uint size);
void fill(void *left, uint size, byte value);
void zero(void *left, uint size);

#define COPY(left, right, count) copy(left, right, count * sizeof(typeof(*left)))
#define MOVE(left, right, count) move(left, right, count * sizeof(typeof(*left)))
#define FILL(left, count, value) fill(left, count * sizeof(typeof(*left)), value)
#define ZERO(left, count)        zero(left, count * sizeof(typeof(*left)))

sintb decode_utf8 (utf32 *left, const utf8  *right);
sintb decode_utf16(utf32 *left, const utf16 *right);

sintb encode_utf8 (utf8  *left, utf32 right);
sintb encode_utf16(utf16 *left, utf32 right);

sintl make_utf8_text_from_utf16(utf8  *left, const utf16 *right);
sintl make_utf16_text_from_utf8(utf16 *left, const utf8  *right);

uint get_size_of_utf8_text (const utf8  *text);
uint get_size_of_utf16_text(const utf16 *text);

sint compare_text      (const utf8 *left, const utf8 *right);
sint compare_sized_text(const utf8 *left, const utf8 *right, uint size);

#define COMPARE_LITERAL_TEXT(left, right) compare_sized_text(left, right, sizeof(right))

void copy_text      (utf8 *left, const utf8 *right);
void copy_sized_text(utf8 *left, const utf8 *right, uint size);

#define COPY_LITERAL_TEXT(left, right) copy_sized_text(left, right, sizeof(right))

sintl format_text_v(utf8 *buffer, uint size, const utf8 *format, vargs vargs);
sintl format_text  (utf8 *buffer, uint size, const utf8 *format, ...);

typedef jmp_buf landing;

#define SET_LANDING(...) setjmp(__VA_ARGS__)

[[noreturn]]
void jump(landing landing, int status);

typedef enum : uintb
{
	// they all have equal width ! how neat !
	severity_verbose,
	severity_comment,
	severity_caution,
	severity_failure,
} severity;

constexpr utf8 severity_representations[][8] =
{
	[severity_verbose] = "VERBOSE",
	[severity_comment] = "COMMENT",
	[severity_caution] = "CAUTION",
	[severity_failure] = "FAILURE",
};

void _report(const char *file, uint line, severity severity, const char *message, ...);

#define REPORT(...) _report(__FILE__, __LINE__, __VA_ARGS__)
#define REPORT_VERBOSE(...) REPORT(severity_verbose, __VA_ARGS__)
#define REPORT_COMMENT(...) REPORT(severity_comment, __VA_ARGS__)
#define REPORT_CAUTION(...) REPORT(severity_caution, __VA_ARGS__)
#define REPORT_FAILURE(...) REPORT(severity_failure, __VA_ARGS__)

constexpr uint memory_page_size = 4096;
constexpr uint universal_alignment = alignof(max_align_t);

void *allocate(uint size);

void deallocate(void *memory, uint size);

typedef struct region region;
struct region
{
	uint    size;
	uint    mass;
	byte   *view;
	region *prior;
	region *next;

	alignas(universal_alignment) byte data[];
};

constexpr uint default_minimum_region_size_of_regional_allocator = memory_page_size - sizeof(region);

typedef struct regional_allocator regional_allocator;
struct regional_allocator
{
	//allocator *allocator;
	uint       minimum_region_size;

	region *active_region;
	region *first_region;
};

void *push(uint size, uint alignment, regional_allocator *allocator);

#define PUSH(type, count, allocator)      (type *)push(count * sizeof(type), alignof(type), allocator)
#define PUSH_TRAIN(head, body, allocator) (head *)push(sizeof(head) + sizeof(body), alignof(head), allocator)

extern thread_local struct context
{
	regional_allocator allocators[2];

	landing default_failure_landing;
	landing *failure_landing;
} context;

extern struct base
{
	regional_allocator persistent_allocator;

	utf8 *command_line;
	uint  command_line_size;
} base;

uintl get_time(void);

constexpr uint maximum_size_of_path = MAX_PATH;

typedef void *file_handle;

uint get_current_directory_path(utf8 *path);

file_handle create_file(const utf8 *path);
file_handle open_file  (const utf8 *path);

uintl get_size_of_file(file_handle handle);

uint read_from_file(void *buffer, uint size, file_handle handle);

void close_file(file_handle handle);

#endif

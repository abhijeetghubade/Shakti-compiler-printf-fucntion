#include <stdarg.h>
#include <stdint.h>

#define DESC(d) ((void *)d)

typedef unsigned long uint64_t;

extern volatile uint64_t tohost;
extern volatile uint64_t fromhost;
extern int _prf(int (*func)(), void *dest,
				const char *format, va_list vargs);
typedef unsigned size_t;

void __attribute__((noreturn)) tohost_exit(uintptr_t code)
{
  tohost = (code << 1) | 1;
  while (1);
}


void __attribute__((weak)) thread_entry(int cid, int nc)
{
  // multi-threaded programs override this function.
  // for the case of single-threaded programs, only let core 0 proceed.
  while (cid != 0);
}

static void init_tls()
{
  register void* thread_pointer asm("tp");
  extern char _tls_data;
  extern __thread char _tdata_begin, _tdata_end, _tbss_end;
  size_t tdata_size = &_tdata_end - &_tdata_begin;
  memcpy(thread_pointer, &_tls_data, tdata_size);
  size_t tbss_size = &_tbss_end - &_tdata_end;
  memset(thread_pointer + tdata_size, 0, tbss_size);
}

int uart_init()
{
  asm volatile ("li t1, 0x11200" "\n\t"	//The base address of UART config registers
      "li t2, 0x83" "\n\t"	//Access divisor registers
      "sb t2, 24(t1)" "\n\t"	//Writing to UART_ADDR_LINE_CTRL
      "li t2, 0x0"	"\n\t"	//The upper bits of uart div
      "sb x0, 8(t1)" "\n\t"	//Storing upper bits of uart div
      "li t2, 0x10F" "\n\t"	//The lower bits of uart div //20MHz 9600baud
      "sb t2, 0(t1)" "\n\t"			
      "li t2, 0x3" "\n\t"
      "sb t2, 24(t1)" "\n\t"
      "li t2, 0x6" "\n\t"
      "sb t2, 16(t1)" "\n\t"
      "sb x0, 8(t1)" "\n\n"
      :
      :
      :"x0","t1","t2", "cc", "memory");
  return 0;
}
#undef getchar
int getchar()                                                              
{

  register char a0 asm("a0");
  asm volatile ("li t1, 0x11300" "\n\t" //The base address of UART config registers
      "uart_statusr: lb t2, 40(t1)" "\n\t"
      "andi t2, t2, 0x1" "\n\t"
      "beqz t2, uart_statusr" "\n\t"
      "lb a0, 0(t1)"  "\n\t"      //The base address of UART data register
      :  
      :  
      :"a0","t1","t2","cc","memory");

  return a0;
}


#undef putchar
int putchar(int ch)
{
  register char a0 asm("a0") = ch;
  asm volatile ("li t1, 0x11300" "\n\t"	//The base address of UART config registers
        "uart_status_simple: lb a1, 12(t1)" "\n\t"
        "andi a1,a1,0x2" "\n\t"
        "beqz a1, uart_status_simple" "\n\t"
				"sb a0, 4(t1)"  "\n\t"
				:
				:
				:"a0","t1","cc","memory");
  return 0;
}



void _init(int cid, int nc)
{
  init_tls();
  thread_entry(cid, nc);
  // only single-threaded programs should ever get here.
  int ret = main(0, 0);


    exit(ret);
}


uintptr_t __attribute__((weak)) handle_trap(uintptr_t cause, uintptr_t epc, uintptr_t regs[32])
{
  tohost_exit(1337);
}



int fputc(int c, int *stream)
{
        return putchar(c);
}


int printf(const char * format, ...)
{
	va_list vargs;
	int     r;

	va_start(vargs, format);
	r = _prf(fputc, 0, format, vargs);
	va_end(vargs);

	return r;
}

int main()
{
printf("%s","hello \n");
return 0;
}

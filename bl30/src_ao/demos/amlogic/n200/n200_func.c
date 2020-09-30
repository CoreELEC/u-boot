// See LICENSE for license details.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "riscv_encoding.h"
#include "n200_func.h"
#include "register.h"
#include "common.h"
#include "n200_timer.h"

    // Configure PMP to make all the address space accesable and executable
void pmp_open_all_space(void){
    // Config entry0 addr to all 1s to make the range cover all space
    asm volatile ("li x6, 0xffffffff":::"x6");
    asm volatile ("csrw pmpaddr0, x6":::);
    // Config entry0 cfg to make it NAPOT address mode, and R/W/X okay
    asm volatile ("li x6, 0x7f":::"x6");
    asm volatile ("csrw pmpcfg0, x6":::);
}

void switch_m2u_mode(void){
    clear_csr (mstatus,MSTATUS_MPP);
    //printf("\nIn the m2u function, the mstatus is 0x%x\n", read_csr(mstatus));
    //printf("\nIn the m2u function, the mepc is 0x%x\n", read_csr(mepc));
    asm volatile ("la x6, 1f    ":::"x6");
    asm volatile ("csrw mepc, x6":::);
    asm volatile ("mret":::);
    asm volatile ("1:":::);
}

uint32_t mtime_lo(void)
{
  return *(volatile uint32_t *)(TIMER_CTRL_ADDR + TIMER_MTIME);
}


uint32_t mtime_hi(void)
{
  return *(volatile uint32_t *)(TIMER_CTRL_ADDR + TIMER_MTIME + 4);
}

uint64_t get_timer_value(void)
{
  while (1) {
    uint32_t hi = mtime_hi();
    uint32_t lo = mtime_lo();
    if (hi == mtime_hi())
      return ((uint64_t)hi << 32) | lo;
  }
}

uint32_t get_timer_freq(void)
{
  return TIMER_FREQ;
}

uint64_t get_instret_value(void)
{
  while (1) {
    uint32_t hi = read_csr(minstreth);
    uint32_t lo = read_csr(minstret);
    if (hi == read_csr(minstreth))
      return ((uint64_t)hi << 32) | lo;
  }
}

uint64_t get_cycle_value(void)
{
  while (1) {
    uint32_t hi = read_csr(mcycleh);
    uint32_t lo = read_csr(mcycle);
    if (hi == read_csr(mcycleh))
      return ((uint64_t)hi << 32) | lo;
  }
}

unsigned long interrupt_status_get(void)
{
	return read_csr(mstatus) >> 0x3;
}

void interrupt_disable(void)
{
	clear_csr(mstatus, MSTATUS_MIE);
}

void interrupt_enable(void)
{
	set_csr(mstatus, MSTATUS_MIE);
}

#ifndef N200_REVA

uint32_t __attribute__((noinline)) measure_cpu_freq(size_t n)
{
  uint32_t start_mtime, delta_mtime;
  uint32_t mtime_freq = get_timer_freq();

  // Don't start measuruing until we see an mtime tick
  uint32_t tmp = mtime_lo();
  do {
    start_mtime = mtime_lo();
  } while (start_mtime == tmp);

  uint32_t start_mcycle = read_csr(mcycle);

  do {
    delta_mtime = mtime_lo() - start_mtime;
  } while (delta_mtime < n);

  uint32_t delta_mcycle = read_csr(mcycle) - start_mcycle;

  return (delta_mcycle / delta_mtime) * mtime_freq
         + ((delta_mcycle % delta_mtime) * mtime_freq) / delta_mtime;
}

uint32_t get_cpu_freq(void)
{
  uint32_t cpu_freq;

  // warm up
  measure_cpu_freq(1);
  // measure for real
  cpu_freq = measure_cpu_freq(100);

  return cpu_freq;
}

unsigned int xPortIsIsrContext(void)
{
	return (read_csr_msubmode & 0xff);
}

// Note that there are no assertions or bounds checking on these
// parameter values.


void eclic_init ( uint32_t num_irq )
{

  typedef volatile uint32_t vuint32_t;

  //clear cfg register
  *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_CFG_OFFSET) = 0;

  //clear minthresh register
  *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_MTH_OFFSET) = 0;

  //clear all IP/IE/ATTR/CTRL bits for all interrupt sources
  vuint32_t * ptr;

  vuint32_t * base = (vuint32_t*)(ECLIC_ADDR_BASE + ECLIC_INT_IP_OFFSET);
  vuint32_t * upper = (vuint32_t*)(base + num_irq*4);

  for (ptr = base; ptr < upper; ptr=ptr+4) {
    *ptr = 0;
  }

  clean_int_src();
}

void print_eclic(void)
{
	typedef volatile uint32_t vuint32_t;

	vuint32_t * ptr = (vuint32_t*)(ECLIC_ADDR_BASE + ECLIC_INT_IP_OFFSET + 7*4);

	printf("\nTIME=0x%lx\n",*ptr);
}


void eclic_enable_interrupt (uint32_t source) {
    *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_IE_OFFSET+source*4) = 1;
}

void eclic_disable_interrupt (uint32_t source){
    *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_IE_OFFSET+source*4) = 0;
}

void eclic_set_pending(uint32_t source){
    *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_IP_OFFSET+source*4) = 1;
}

void eclic_clear_pending(uint32_t source){
    *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_IP_OFFSET+source*4) = 0;
}

void eclic_set_intctrl (uint32_t source, uint8_t intctrl){
  *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_CTRL_OFFSET+source*4) = intctrl;
}

uint8_t eclic_get_intctrl  (uint32_t source){
  return *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_CTRL_OFFSET+source*4);
}

void eclic_set_intattr (uint32_t source, uint8_t intattr){
  *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_ATTR_OFFSET+source*4) = intattr;
}

uint8_t eclic_get_intattr  (uint32_t source){
  return *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_INT_ATTR_OFFSET+source*4);
}

void eclic_set_cliccfg (uint8_t cliccfg){
  *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_CFG_OFFSET) = cliccfg;
}

uint8_t eclic_get_cliccfg (void){
  return *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_CFG_OFFSET);
}

void eclic_set_mth (uint8_t mth){
  *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_MTH_OFFSET) = mth;
}

uint8_t eclic_get_mth  (void){
  return *(volatile uint8_t*)(ECLIC_ADDR_BASE+ECLIC_MTH_OFFSET);
}

//sets nlbits
void eclic_set_nlbits(uint8_t nlbits) {
  //shift nlbits to correct position
  uint8_t nlbits_shifted = nlbits << ECLIC_CFG_NLBITS_LSB;

  //read the current cliccfg
  uint8_t old_cliccfg = eclic_get_cliccfg();
  uint8_t new_cliccfg = (old_cliccfg & (~ECLIC_CFG_NLBITS_MASK)) | (ECLIC_CFG_NLBITS_MASK & nlbits_shifted);

  eclic_set_cliccfg(new_cliccfg);
}

//get nlbits
uint8_t eclic_get_nlbits(void) {
  //extract nlbits
  uint8_t nlbits = eclic_get_cliccfg();
  nlbits = (nlbits & ECLIC_CFG_NLBITS_MASK) >> ECLIC_CFG_NLBITS_LSB;
  return nlbits;
}

//sets an interrupt level based encoding of nlbits and CLICINTCTLBITS
void eclic_set_irq_lvl(uint32_t source, uint8_t lvl) {
  //extract nlbits
  uint8_t nlbits = eclic_get_nlbits();
  if (nlbits > CLICINTCTLBITS) {
    nlbits = CLICINTCTLBITS;
  }

  //shift lvl right to mask off unused bits
  lvl = lvl >> (8-nlbits);
  //shift lvl into correct bit position
  lvl = lvl << (8-nlbits);

  //write to clicintctrl
  uint8_t current_intctrl = eclic_get_intctrl(source);
  //shift intctrl left to mask off unused bits
  current_intctrl = current_intctrl << nlbits;
  //shift intctrl into correct bit position
  current_intctrl = current_intctrl >> nlbits;

  eclic_set_intctrl(source, (current_intctrl | lvl));
}

//gets an interrupt level based encoding of nlbits
uint8_t eclic_get_irq_lvl(uint32_t source) {
  //extract nlbits
  uint8_t nlbits = eclic_get_nlbits();
  if (nlbits > CLICINTCTLBITS) {
    nlbits = CLICINTCTLBITS;
  }

  uint8_t intctrl = eclic_get_intctrl(source);

  //shift intctrl
  intctrl = intctrl >> (8-nlbits);
  //shift intctrl
  uint8_t lvl = intctrl << (8-nlbits);

  return lvl;
}

void eclic_set_irq_lvl_abs(uint32_t source, uint8_t lvl_abs) {
  //extract nlbits
  uint8_t nlbits = eclic_get_nlbits();
  if (nlbits > CLICINTCTLBITS) {
    nlbits = CLICINTCTLBITS;
  }

  //shift lvl_abs into correct bit position
  uint8_t lvl = lvl_abs << (8-nlbits);

  //write to clicintctrl
  uint8_t current_intctrl = eclic_get_intctrl(source);
  //shift intctrl left to mask off unused bits
  current_intctrl = current_intctrl << nlbits;
  //shift intctrl into correct bit position
  current_intctrl = current_intctrl >> nlbits;

  eclic_set_intctrl(source, (current_intctrl | lvl));
}

uint8_t eclic_get_irq_lvl_abs(uint32_t source) {
  //extract nlbits
  uint8_t nlbits = eclic_get_nlbits();
  if (nlbits > CLICINTCTLBITS) {
    nlbits = CLICINTCTLBITS;
  }

  uint8_t intctrl = eclic_get_intctrl(source);

  //shift intctrl
  intctrl = intctrl >> (8-nlbits);
  //shift intctrl
  uint8_t lvl_abs = intctrl;

  return lvl_abs;
}

void eclic_set_irq_pri(uint32_t source, uint8_t pri) {
  //extract nlbits
  uint8_t nlbits = eclic_get_nlbits();
  if (nlbits > CLICINTCTLBITS) {
    nlbits = CLICINTCTLBITS;
  }

  //write to clicintctrl
  uint8_t current_intctrl = eclic_get_intctrl(source);
  //shift intctrl left to mask off unused bits
  current_intctrl = current_intctrl >> (8 - nlbits);
  //shift intctrl into correct bit position
  current_intctrl = current_intctrl << (8 - nlbits);

  eclic_set_intctrl(source, (current_intctrl | pri));
}

void eclic_mode_enable(void) {
  uint32_t mtvec_value = read_csr(mtvec);
  mtvec_value = mtvec_value & 0xFFFFFFC0;
  mtvec_value = mtvec_value | 0x00000003;
  write_csr(mtvec,mtvec_value);
}

//sets vector-mode or non-vector mode
void eclic_set_vmode(uint32_t source) {
  //read the current attr
  uint8_t old_intattr = eclic_get_intattr(source);
      // Keep other bits unchanged and only set the LSB bit
  uint8_t new_intattr = (old_intattr | 0x1);

  eclic_set_intattr(source,new_intattr);
}

void eclic_set_nonvmode(uint32_t source) {
  //read the current attr
  uint8_t old_intattr = eclic_get_intattr(source);
      // Keep other bits unchanged and only clear the LSB bit
  uint8_t new_intattr = (old_intattr & (~0x1));

  eclic_set_intattr(source,new_intattr);
}

//sets interrupt as level sensitive
//Bit 1, trig[0], is defined as "edge-triggered" (0: level-triggered, 1: edge-triggered);
//Bit 2, trig[1], is defined as "negative-edge" (0: positive-edge, 1: negative-edge).

void eclic_set_level_trig(uint32_t source) {
  //read the current attr
  uint8_t old_intattr = eclic_get_intattr(source);
      // Keep other bits unchanged and only clear the bit 1
  uint8_t new_intattr = (old_intattr & (~0x2));

  eclic_set_intattr(source,new_intattr);
}

void eclic_set_posedge_trig(uint32_t source) {
  //read the current attr
  uint8_t old_intattr = eclic_get_intattr(source);
      // Keep other bits unchanged and only set the bit 1
  uint8_t new_intattr = (old_intattr | 0x2);
      // Keep other bits unchanged and only clear the bit 2
  new_intattr = (new_intattr & (~0x4));

  eclic_set_intattr(source,new_intattr);
}

void eclic_set_negedge_trig(uint32_t source) {
  //read the current attr
  uint8_t old_intattr = eclic_get_intattr(source);
      // Keep other bits unchanged and only set the bit 1
  uint8_t new_intattr = (old_intattr | 0x2);
      // Keep other bits unchanged and only set the bit 2
  new_intattr = (new_intattr | 0x4);

  eclic_set_intattr(source,new_intattr);
}

extern void core_wfe(void);
void wfe(void) {
  core_wfe();
}

void clean_int_src(void)
{
	for (uint32_t i=0; i<8; i++)
		REG32(AOCPU_IRQ_SEL0 + i*4) = 0;
}

int int_src_sel(uint32_t ulIrq, uint32_t src)
{
	uint32_t index;

	if (ulIrq < ECLIC_INTERNAL_NUM_INTERRUPTS ||
		ulIrq > ECLIC_NUM_INTERRUPTS) {
		printf("Error ulIrq!\n");
		return -1;
	}

	if (src > 0xff) {
		printf("Error src!\n");
		return -2;
	}

	ulIrq -= ECLIC_INTERNAL_NUM_INTERRUPTS;

	index = ulIrq/4;
	REG32(AOCPU_IRQ_SEL0 + index*4) &= ~(0xff << (ulIrq%4)*8);
	REG32(AOCPU_IRQ_SEL0 + index*4) |= src << (ulIrq%4)*8;
	return 0;
}

int int_src_clean(uint32_t ulIrq)
{
	uint32_t index;

	if (ulIrq < ECLIC_INTERNAL_NUM_INTERRUPTS ||
		ulIrq > ECLIC_NUM_INTERRUPTS) {
		printf("Error ulIrq!\n");
		return -1;
	}

	ulIrq -= ECLIC_INTERNAL_NUM_INTERRUPTS;

	index = ulIrq/4;
	REG32(AOCPU_IRQ_SEL0 + index*4) &= ~(0xff << (ulIrq%4)*8);
	return 0;
}

/*Just for external interrupt source.
 *Because int_src_sel() just support external select
 */
int eclic_map_interrupt(uint32_t ulIrq, uint32_t src)
{
	uint8_t val;

	if (int_src_sel(ulIrq, src)) {
		printf("Enable %ld irq, %ld src fail!\n", ulIrq, src);
		return -1;
	}

	val = eclic_get_intattr (ulIrq);
	val |= ECLIC_INT_ATTR_MACH_MODE;
	/*Use edge trig interrupt default*/
	val |= ECLIC_INT_ATTR_TRIG_EDGE;
	eclic_set_intattr(ulIrq, val);
	//eclic_enable_interrupt(ulIrq);
	return 0;
}

int eclic_interrupt_inner[SOC_ECLIC_NUM_INTERRUPTS] = {0};
extern uint32_t vector_base;

int RegisterIrq(uint32_t int_num, uint32_t int_priority, function_ptr_t handler) {
	int irq = 0;

	for (irq = ECLIC_INTERNAL_NUM_INTERRUPTS; irq <= ECLIC_NUM_INTERRUPTS; irq ++) {
		if (eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] == 0)
			break;
	}
	if (eclic_map_interrupt(irq, int_num) < 0) {
		printf("eclic map error.\n");
		return -1;
	}
	eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] = int_num;

	*(&vector_base + irq) = handler;
	eclic_set_irq_pri(irq, int_priority);

	return 0;
}

int UnRegisterIrq(uint32_t ulIrq)
{
	int irq = 0;
	for (irq = ECLIC_INTERNAL_NUM_INTERRUPTS; irq <= ECLIC_NUM_INTERRUPTS; irq ++) {
		if (eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] == ulIrq)
			break;
	}
	if (irq > ECLIC_NUM_INTERRUPTS) {
		printf("Error ulIrq!\n");
		return -1;
	} else {
		if (int_src_clean(irq)) {
			printf("unregister %ld irq, %ld src fail!\n", ulIrq, irq);
			return -1;
		}
		eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] = 0;
		*(&vector_base + irq) = 0;
	}
	return 0;
}

int EnableIrq(uint32_t ulIrq)
{
	int irq = 0;
	for (irq = ECLIC_INTERNAL_NUM_INTERRUPTS; irq <= ECLIC_NUM_INTERRUPTS; irq ++) {
		if (eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] == ulIrq)
			break;
	}
	if (irq > ECLIC_NUM_INTERRUPTS) {
		printf("Error ulIrq!\n");
		return -1;
	} else {
		eclic_enable_interrupt(irq);
	}
	return 0;
}

int DisableIrq(uint32_t ulIrq)
{
	int irq = 0;
	for (irq = ECLIC_INTERNAL_NUM_INTERRUPTS; irq <= ECLIC_NUM_INTERRUPTS; irq ++) {
		if (eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] == ulIrq)
			break;
	}
	if (irq > ECLIC_NUM_INTERRUPTS) {
		printf("Error ulIrq!\n");
		return -1;
	} else {
		eclic_disable_interrupt(irq);
	}
	return 0;
}

int SetIrqPriority(uint32_t ulIrq, uint32_t ulProi)
{
	int irq = 0;
	for (irq = ECLIC_INTERNAL_NUM_INTERRUPTS; irq <= ECLIC_NUM_INTERRUPTS; irq ++) {
		if (eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] == ulIrq)
			break;
	}
	if (irq > ECLIC_NUM_INTERRUPTS) {
		printf("Error ulIrq!\n");
		return -1;
	} else {
		eclic_set_irq_pri(irq, ulProi);
	}
	return 0;
}

int ClearPendingIrq(uint32_t ulIrq)
{
	int irq = 0;
	for (irq = ECLIC_INTERNAL_NUM_INTERRUPTS; irq <= ECLIC_NUM_INTERRUPTS; irq ++) {
		if (eclic_interrupt_inner[irq - ECLIC_INTERNAL_NUM_INTERRUPTS] == ulIrq)
			break;
	}
	if (irq > ECLIC_NUM_INTERRUPTS) {
		printf("Error ulIrq!\n");
		return -1;
	} else {
		eclic_clear_pending(irq);
	}
	return 0;
}
#else
// Note that there are no assertions or bounds checking on these
// parameter values.

void pic_set_threshold(uint32_t threshold)
{
  volatile uint32_t* threshold_ptr = (uint32_t*) (PIC_CTRL_ADDR +
						  PIC_THRESHOLD_OFFSET);

  *threshold_ptr = threshold;
}

void pic_enable_interrupt(uint32_t source)
{
  volatile uint32_t * current_ptr = (volatile uint32_t *)(PIC_CTRL_ADDR +
                                                        PIC_ENABLE_OFFSET +
                                                        ((source >> 3) & (~0x3))//Source number divide 32 and then multip 4 (bytes)
                                                        );
  uint32_t current = *current_ptr;
  current = current | ( 1 << (source & 0x1f));// Only check the least 5 bits
  *current_ptr = current;
}

void pic_disable_interrupt (uint32_t source){

  volatile uint32_t * current_ptr = (volatile uint32_t *) (PIC_CTRL_ADDR +
                                                         PIC_ENABLE_OFFSET +
                                                         ((source >> 3) & (~0x3))//Source number divide 32 and then multip 4 (bytes)
                                                          );
  uint32_t current = *current_ptr;
  current = current & ~(( 1 << (source & 0x1f)));// Only check the least 5 bits
  *current_ptr = current;
}

void pic_set_priority (uint32_t source, uint32_t priority){

  if (PIC_NUM_PRIORITIES > 0) {
    volatile uint32_t * priority_ptr = (volatile uint32_t *)
      (PIC_CTRL_ADDR +
       PIC_PRIORITY_OFFSET +
       (source << PIC_PRIORITY_SHIFT_PER_SOURCE));// Each priority reg occupy a word, so multiple 2
    *priority_ptr = priority;
  }
}

uint32_t pic_claim_interrupt(void){

  volatile uint32_t * claim_addr = (volatile uint32_t * )
    (PIC_CTRL_ADDR +
     PIC_CLAIM_OFFSET
     );

  return  *claim_addr;
}

uint32_t pic_check_eip(void){

  volatile uint32_t * eip_addr = (volatile uint32_t * )
    (PIC_CTRL_ADDR +
     PIC_EIP_OFFSET
     );

  return  *eip_addr;
}

void pic_complete_interrupt(uint32_t source){

  volatile uint32_t * claim_addr = (volatile uint32_t *) (PIC_CTRL_ADDR +
                                                                PIC_CLAIM_OFFSET
                                                                );
  *claim_addr = source;
}

void DefaultInterruptHandler(void)
{
}

int RegisterIrq(uint32_t int_num, uint32_t int_priority, function_ptr_t handler) {
    pic_interrupt_handlers[int_num] = handler;
    pic_set_priority(int_num, int_priority);
   // pic_enable_interrupt (int_num);
    return 0;
}

int UnRegisterIrq(uint32_t int_num)
{
    pic_interrupt_handlers[int_num] = DefaultInterruptHandler;
    pic_set_priority(int_num, 0);

    return 0;
}

int EnableIrq(uint32_t ulIrq)
{
    pic_enable_interrupt(ulIrq);
    return 0;
}

int DisableIrq(uint32_t ulIrq)
{
    pic_disable_interrupt(ulIrq);
    return 0;
}

int SetIrqPriority(uint32_t ulIrq, uint32_t ulPri)
{
    pic_set_priority(ulIrq, ulPri);
    return 0;
}

static unsigned int irq_setting[IRQ_EN_REG_NUM];
/*N205 does not support clear pending irq.*
 *Need use work around to clear pending:  *
 *1. disable irq (MIE)                    *
 *2. store and disable current enable irq *
 *3. enable target irq                    *
 *4. claim and complete target irq        *
 *5. disable target irq                   *
 *6. restore current irq enable setting   *
 *7. enable irq (MIE)
*/
int ClearPendingIrq(uint32_t ulIrq)
{
	unsigned int i;
	volatile uint32_t * current_ptr;
	unsigned long irq_status;

	irq_status = interrupt_status_get();
	if (irq_status)
		interrupt_disable();

	for (i = 0; i < IRQ_EN_REG_NUM; i++)
	{
		current_ptr =
			(volatile uint32_t *)(PIC_CTRL_ADDR + PIC_ENABLE_OFFSET + i*4);
		irq_setting[i] =
			REG32(current_ptr);
		REG32(current_ptr) = 0;
	}
	pic_enable_interrupt(ulIrq);
	i = pic_claim_interrupt();
	if (i)
		pic_complete_interrupt(i);
	pic_disable_interrupt(ulIrq);

	for (i = 0; i < IRQ_EN_REG_NUM; i++)
	{
		current_ptr =
			(volatile uint32_t *)(PIC_CTRL_ADDR + PIC_ENABLE_OFFSET + i*4);
		REG32(current_ptr) = irq_setting[i];
	}

	if (irq_status)
		interrupt_enable();

	return 0;
}

unsigned int xPortIsIsrContext(void)
{
	return read_csr_msubmode;
}

#endif

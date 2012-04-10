/*
 * =====================================================================================
 *
 *       Filename:  detector.h
 *
 *    Description:	Return-Oriented Rootkit - TEMU plugin
 *
 *        Version:  1.0
 *        Created:  03/31/2012 07:00:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:	Do Hoang Nhat Huy
 *        Company:
 *
 * =====================================================================================
 */

#ifndef  DETECTOR_INC
#define  DETECTOR_INC

//! This structure is copied from Hookfinder and can be customized further.
typedef struct {
	// Id of the caller
	uint32_t caller;
	// Id of the callee
	uint32_t callee;
	// EIP register
	uint32_t eip;
	// Still trying to figure out what it is.
	uint32_t depend_id;
} taint_record_t;

//! Everything about an instruction
typedef struct {
	// Raw
	uint8_t raw_insn[16];
	// Registers
	uint32_t eip;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint16_t sp;
	uint16_t bp;
	uint16_t si;
	uint16_t di;
	uint16_t ax;
	uint16_t bx;
	uint16_t cx;
	uint16_t dx;
	uint8_t al;
	uint8_t bl;
	uint8_t cl;
	uint8_t dl;
	uint8_t ah;
	uint8_t bh;
	uint8_t ch;
	uint8_t dh;
	// Top of stack
	uint32_t tos;
} insn_record_t;

#endif   /* ----- #ifndef DETECTOR_INC  ----- */


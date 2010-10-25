/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "radeon_compiler.h"

#include <stdio.h>

#include "radeon_dataflow.h"
#include "radeon_emulate_branches.h"
#include "radeon_emulate_loops.h"
#include "radeon_program_alu.h"
#include "radeon_program_tex.h"
#include "radeon_rename_regs.h"
#include "radeon_remove_constants.h"
#include "r300_fragprog.h"
#include "r300_fragprog_swizzle.h"
#include "r500_fragprog.h"


static void dataflow_outputs_mark_use(void * userdata, void * data,
		void (*callback)(void *, unsigned int, unsigned int))
{
	struct r300_fragment_program_compiler * c = userdata;
	callback(data, c->OutputColor[0], RC_MASK_XYZW);
	callback(data, c->OutputColor[1], RC_MASK_XYZW);
	callback(data, c->OutputColor[2], RC_MASK_XYZW);
	callback(data, c->OutputColor[3], RC_MASK_XYZW);
	callback(data, c->OutputDepth, RC_MASK_W);
}

static void rc_rewrite_depth_out(struct radeon_compiler *cc, void *user)
{
	struct r300_fragment_program_compiler *c = (struct r300_fragment_program_compiler*)cc;
	struct rc_instruction *rci;

	for (rci = c->Base.Program.Instructions.Next; rci != &c->Base.Program.Instructions; rci = rci->Next) {
		struct rc_sub_instruction * inst = &rci->U.I;

		if (inst->DstReg.File != RC_FILE_OUTPUT || inst->DstReg.Index != c->OutputDepth)
			continue;

		if (inst->DstReg.WriteMask & RC_MASK_Z) {
			inst->DstReg.WriteMask = RC_MASK_W;
		} else {
			inst->DstReg.WriteMask = 0;
			continue;
		}

		switch (inst->Opcode) {
			case RC_OPCODE_FRC:
			case RC_OPCODE_MOV:
				inst->SrcReg[0] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[0]);
				break;
			case RC_OPCODE_ADD:
			case RC_OPCODE_MAX:
			case RC_OPCODE_MIN:
			case RC_OPCODE_MUL:
				inst->SrcReg[0] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[0]);
				inst->SrcReg[1] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[1]);
				break;
			case RC_OPCODE_CMP:
			case RC_OPCODE_MAD:
				inst->SrcReg[0] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[0]);
				inst->SrcReg[1] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[1]);
				inst->SrcReg[2] = lmul_swizzle(RC_SWIZZLE_ZZZZ, inst->SrcReg[2]);
				break;
			default:
				// Scalar instructions needn't be reswizzled
				break;
		}
	}
}

void r3xx_compile_fragment_program(struct r300_fragment_program_compiler* c)
{
	int is_r500 = c->Base.is_r500;
	int kill_consts = c->Base.remove_unused_constants;
	int opt = !c->Base.disable_optimizations;

	/* Lists of instruction transformations. */
	struct radeon_program_transformation rewrite_tex[] = {
		{ &radeonTransformTEX, c },
		{ 0, 0 }
	};

	struct radeon_program_transformation native_rewrite_r500[] = {
		{ &r500_transform_IF, 0 },
		{ &radeonTransformALU, 0 },
		{ &radeonTransformDeriv, 0 },
		{ &radeonTransformTrigScale, 0 },
		{ 0, 0 }
	};

	struct radeon_program_transformation native_rewrite_r300[] = {
		{ &radeonTransformALU, 0 },
		{ &r300_transform_trig_simple, 0 },
		{ 0, 0 }
	};

	/* List of compiler passes. */
	struct radeon_compiler_pass fs_list[] = {
		/* NAME				DUMP PREDICATE	FUNCTION			PARAM */
		{"rewrite depth out",		1, 1,		rc_rewrite_depth_out,		NULL},
		/* This transformation needs to be done before any of the IF
		 * instructions are modified. */
		{"transform KILP",		1, 1,		rc_transform_KILP,		NULL},
		{"unroll loops",		1, is_r500,	rc_unroll_loops,		NULL},
		{"transform loops",		1, !is_r500,	rc_transform_loops,		NULL},
		{"emulate branches",		1, !is_r500,	rc_emulate_branches,		NULL},
		{"transform TEX",		1, 1,		rc_local_transform,		rewrite_tex},
		{"native rewrite",		1, is_r500,	rc_local_transform,		native_rewrite_r500},
		{"native rewrite",		1, !is_r500,	rc_local_transform,		native_rewrite_r300},
		{"deadcode",			1, opt,		rc_dataflow_deadcode,		dataflow_outputs_mark_use},
		{"emulate loops",		1, !is_r500,	rc_emulate_loops,		NULL},
		{"dataflow optimize",		1, opt,		rc_optimize,			NULL},
		{"dataflow swizzles",		1, 1,		rc_dataflow_swizzles,		NULL},
		{"dead constants",		1, kill_consts, rc_remove_unused_constants,	&c->code->constants_remap_table},
		/* This pass makes it easier for the scheduler to group TEX
		 * instructions and reduces the chances of creating too
		 * many texture indirections.*/
		{"register rename",		1, !is_r500,	rc_rename_regs,			NULL},
		{"pair translate",		1, 1,		rc_pair_translate,		NULL},
		{"pair scheduling",		1, 1,		rc_pair_schedule,		NULL},
		{"register allocation",		1, opt,		rc_pair_regalloc,		NULL},
		{"dumb register allocation",	1, !opt,	rc_pair_regalloc_inputs_only,	NULL},
		{"final code validation",	0, 1,		rc_validate_final_shader,	NULL},
		{"machine code generation",	0, is_r500,	r500BuildFragmentProgramHwCode,	NULL},
		{"machine code generation",	0, !is_r500,	r300BuildFragmentProgramHwCode,	NULL},
		{"dump machine code",		0, is_r500  && c->Base.Debug, r500FragmentProgramDump, NULL},
		{"dump machine code",		0, !is_r500 && c->Base.Debug, r300FragmentProgramDump, NULL},
		{NULL, 0, 0, NULL, NULL}
	};

	c->Base.SwizzleCaps = c->Base.is_r500 ? &r500_swizzle_caps : &r300_swizzle_caps;

	rc_run_compiler(&c->Base, fs_list, "Fragment Program");

	rc_constants_copy(&c->code->constants, &c->Base.Program.Constants);
}

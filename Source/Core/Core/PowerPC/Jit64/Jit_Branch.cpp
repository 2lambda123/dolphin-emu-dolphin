// Copyright 2013 Dolphin Emulator Project
// Licensed under GPLv2
// Refer to the license.txt file included.

#include <initializer_list>
#include "Common/CommonTypes.h"

#include "Core/PowerPC/Jit64/Jit.h"
#include "Core/PowerPC/Jit64/JitAsm.h"
#include "Core/PowerPC/Jit64/JitRegCache.h"

// The branches are known good, or at least reasonably good.
// No need for a disable-mechanism.

// If defined, clears CR0 at blr and bl-s. If the assumption that
// flags never carry over between functions holds, then the task for
// an optimizer becomes much easier.

// #define ACID_TEST

// Zelda and many more games seem to pass the Acid Test.

using namespace Gen;

void Jit64::sc(UGeckoInstruction inst)
{
	INSTRUCTION_START
	JITDISABLE(bJITBranchOff);

	gpr.Flush();
	fpr.Flush();
	MOV(32, PPCSTATE(pc), Imm32(js.compilerPC + 4));
	LOCK();
	OR(32, PPCSTATE(Exceptions), Imm32(EXCEPTION_SYSCALL));
	WriteExceptionExit();
}

void Jit64::rfi(UGeckoInstruction inst)
{
	INSTRUCTION_START
	JITDISABLE(bJITBranchOff);

	gpr.Flush();
	fpr.Flush();
	// See Interpreter rfi for details
	const u32 mask = 0x87C0FFFF;
	const u32 clearMSR13 = 0xFFFBFFFF; // Mask used to clear the bit MSR[13]
	// MSR = ((MSR & ~mask) | (SRR1 & mask)) & clearMSR13;
	AND(32, PPCSTATE(msr), Imm32((~mask) & clearMSR13));
	MOV(32, R(RSCRATCH), PPCSTATE_SRR1);
	AND(32, R(RSCRATCH), Imm32(mask & clearMSR13));
	OR(32, PPCSTATE(msr), R(RSCRATCH));
	// NPC = SRR0;
	MOV(32, R(RSCRATCH), PPCSTATE_SRR0);
	WriteRfiExitDestInRSCRATCH();
}

void Jit64::bx(UGeckoInstruction inst)
{
	INSTRUCTION_START
	JITDISABLE(bJITBranchOff);

	// We must always process the following sentence
	// even if the blocks are merged by PPCAnalyst::Flatten().
	if (inst.LK)
		MOV(32, PPCSTATE_LR, Imm32(js.compilerPC + 4));

	// If this is not the last instruction of a block,
	// we will skip the rest process.
	// Because PPCAnalyst::Flatten() merged the blocks.
	if (!js.isLastInstruction)
	{
		return;
	}

	gpr.Flush();
	fpr.Flush();

	u32 destination;
	if (inst.AA)
		destination = SignExt26(inst.LI << 2);
	else
		destination = js.compilerPC + SignExt26(inst.LI << 2);
#ifdef ACID_TEST
	if (inst.LK)
		AND(32, PPCSTATE(cr), Imm32(~(0xFF000000)));
#endif
	if (destination == js.compilerPC)
	{
		//PanicAlert("Idle loop detected at %08x", destination);
		// CALL(ProtectFunction(&CoreTiming::Idle, 0));
		// JMP(Asm::testExceptions, true);
		// make idle loops go faster
		js.downcountAmount += 8;
	}
	WriteExit(destination, inst.LK, js.compilerPC + 4);
}

// TODO - optimize to hell and beyond
// TODO - make nice easy to optimize special cases for the most common
// variants of this instruction.
void Jit64::bcx(UGeckoInstruction inst)
{
	INSTRUCTION_START
	JITDISABLE(bJITBranchOff);

	u32 destination;
	if (inst.AA)
		destination = SignExt16(inst.BD << 2);
	else
		destination = js.compilerPC + SignExt16(inst.BD << 2);

	bool cc = analyzer.HasOption(PPCAnalyst::PPCAnalyzer::OPTION_CONDITIONAL_CONTINUE);
	bool forwardJumps = analyzer.HasOption(PPCAnalyst::PPCAnalyzer::OPTION_FORWARD_JUMP);
	bool jumpInBlock = cc && forwardJumps && destination > js.compilerPC && destination < js.blockEnd;
	int branchCount = 0;
	FixupBranch branch[2];

	if ((inst.BO & BO_DONT_DECREMENT_FLAG) == 0)  // Decrement and test CTR
	{
		bool dir = jumpInBlock ^ !!(inst.BO & BO_BRANCH_IF_CTR_0);
		SUB(32, PPCSTATE_CTR, Imm8(1));
		branch[branchCount++] = J_CC(dir ? CC_NZ : CC_Z, true);
	}

	FixupBranch pConditionBranch = { NULL, 0 };
	if ((inst.BO & BO_DONT_CHECK_CONDITION) == 0)  // Test a CR bit
	{
		bool dir = jumpInBlock ^ !(inst.BO_2 & BO_BRANCH_IF_TRUE);
		branch[branchCount++] = JumpIfCRFieldBit(inst.BI >> 2, 3 - (inst.BI & 3), dir);
	}

	if (jumpInBlock)
	{
		BranchTarget branchData = { {}, branchCount, js.downcountAmount, js.fifoBytesThisBlock, js.firstFPInstructionFound, gpr, fpr, js.op };
		for (int i = 0; i < branchCount; i++)
			branchData.sourceBranch[i] = branch[i];
		branch_targets.insert(std::make_pair(destination, branchData));
	}
	else
	{
		if (inst.LK)
			MOV(32, PPCSTATE_LR, Imm32(js.compilerPC + 4));

		gpr.Flush(FLUSH_MAINTAIN_STATE);
		fpr.Flush(FLUSH_MAINTAIN_STATE);
		WriteExit(destination, inst.LK, js.compilerPC + 4);

		while (branchCount > 0)
			SetJumpTarget(branch[--branchCount]);
	}

	if (!cc)
	{
		gpr.Flush();
		fpr.Flush();
		WriteExit(js.compilerPC + 4);
	}
}

void Jit64::bcctrx(UGeckoInstruction inst)
{
	INSTRUCTION_START
	JITDISABLE(bJITBranchOff);

	// bcctrx doesn't decrement and/or test CTR
	_dbg_assert_msg_(POWERPC, inst.BO_2 & BO_DONT_DECREMENT_FLAG, "bcctrx with decrement and test CTR option is invalid!");

	if (inst.BO_2 & BO_DONT_CHECK_CONDITION)
	{
		// BO_2 == 1z1zz -> b always

		//NPC = CTR & 0xfffffffc;
		gpr.Flush();
		fpr.Flush();

		MOV(32, R(RSCRATCH), PPCSTATE_CTR);
		if (inst.LK_3)
			MOV(32, PPCSTATE_LR, Imm32(js.compilerPC + 4)); // LR = PC + 4;
		AND(32, R(RSCRATCH), Imm32(0xFFFFFFFC));
		WriteExitDestInRSCRATCH(inst.LK_3, js.compilerPC + 4);
	}
	else
	{
		// Rare condition seen in (just some versions of?) Nintendo's NES Emulator

		// BO_2 == 001zy -> b if false
		// BO_2 == 011zy -> b if true

		FixupBranch b = JumpIfCRFieldBit(inst.BI >> 2, 3 - (inst.BI & 3),
		                                 !(inst.BO_2 & BO_BRANCH_IF_TRUE));
		MOV(32, R(RSCRATCH), PPCSTATE_CTR);
		AND(32, R(RSCRATCH), Imm32(0xFFFFFFFC));
		//MOV(32, PPCSTATE(pc), R(RSCRATCH)); => Already done in WriteExitDestInRSCRATCH()
		if (inst.LK_3)
			MOV(32, PPCSTATE_LR, Imm32(js.compilerPC + 4)); // LR = PC + 4;

		gpr.Flush(FLUSH_MAINTAIN_STATE);
		fpr.Flush(FLUSH_MAINTAIN_STATE);
		WriteExitDestInRSCRATCH(inst.LK_3, js.compilerPC + 4);
		// Would really like to continue the block here, but it ends. TODO.
		SetJumpTarget(b);

		if (!analyzer.HasOption(PPCAnalyst::PPCAnalyzer::OPTION_CONDITIONAL_CONTINUE))
		{
			gpr.Flush();
			fpr.Flush();
			WriteExit(js.compilerPC + 4);
		}
	}
}

void Jit64::bclrx(UGeckoInstruction inst)
{
	INSTRUCTION_START
	JITDISABLE(bJITBranchOff);

	FixupBranch pCTRDontBranch;
	if ((inst.BO & BO_DONT_DECREMENT_FLAG) == 0)  // Decrement and test CTR
	{
		SUB(32, PPCSTATE_CTR, Imm8(1));
		if (inst.BO & BO_BRANCH_IF_CTR_0)
			pCTRDontBranch = J_CC(CC_NZ, true);
		else
			pCTRDontBranch = J_CC(CC_Z, true);
	}

	FixupBranch pConditionDontBranch;
	if ((inst.BO & BO_DONT_CHECK_CONDITION) == 0)  // Test a CR bit
	{
		pConditionDontBranch = JumpIfCRFieldBit(inst.BI >> 2, 3 - (inst.BI & 3),
		                                        !(inst.BO_2 & BO_BRANCH_IF_TRUE));
	}

	// This below line can be used to prove that blr "eats flags" in practice.
	// This observation could let us do some useful optimizations.
#ifdef ACID_TEST
	AND(32, PPCSTATE(cr), Imm32(~(0xFF000000)));
#endif

	MOV(32, R(RSCRATCH), PPCSTATE_LR);
	// We don't have to do this because WriteBLRExit handles it for us. Specifically, since we only ever push
	// divisible-by-four instruction addresses onto the stack, if the return address matches, we're already
	// good. If it doesn't match, the mispredicted-BLR code handles the fixup.
	if (!m_enable_blr_optimization)
		AND(32, R(RSCRATCH), Imm32(0xFFFFFFFC));
	if (inst.LK)
		MOV(32, PPCSTATE_LR, Imm32(js.compilerPC + 4));

	gpr.Flush(FLUSH_MAINTAIN_STATE);
	fpr.Flush(FLUSH_MAINTAIN_STATE);
	WriteBLRExit();

	if ((inst.BO & BO_DONT_CHECK_CONDITION) == 0)
		SetJumpTarget( pConditionDontBranch );
	if ((inst.BO & BO_DONT_DECREMENT_FLAG) == 0)
		SetJumpTarget( pCTRDontBranch );

	if (!analyzer.HasOption(PPCAnalyst::PPCAnalyzer::OPTION_CONDITIONAL_CONTINUE))
	{
		gpr.Flush();
		fpr.Flush();
		WriteExit(js.compilerPC + 4);
	}
}

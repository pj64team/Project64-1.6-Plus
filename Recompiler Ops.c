/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and 
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */
#include <Windows.h>
#include <stdio.h>
#include "main.h"
#include "cpu.h"
#include "x86.h"
#include "debugger.h"

DWORD BranchCompare = 0;

void CompileReadTLBMiss (BLOCK_SECTION * Section, int AddressReg, int LookUpReg ) {
	MoveX86regToVariable(AddressReg,&TLBLoadAddress,"TLBLoadAddress");
	TestX86RegToX86Reg(LookUpReg,LookUpReg);
	CompileExit(Section->CompilePC,Section->RegWorking,TLBReadMiss,FALSE,JeLabel32);
}
void CompileWriteTLBMiss(BLOCK_SECTION* Section, int AddressReg, int LookUpReg) {
	MoveX86regToVariable(AddressReg, &TLBLoadAddress, "TLBLoadAddress");
	TestX86RegToX86Reg(LookUpReg, LookUpReg);
	CompileExit(Section->CompilePC, Section->RegWorking, TLBWriteMiss, FALSE, JeLabel32);
}
/************************** Branch functions  ************************/
void Compile_R4300i_Branch (BLOCK_SECTION * Section, void (*CompareFunc)(BLOCK_SECTION * Section), int BranchType, BOOL Link) {
	static int EffectDelaySlot, DoneJumpDelay, DoneContinueDelay;
	static char ContLabel[100], JumpLabel[100];
	static REG_INFO RegBeforeDelay;
	int count;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		
		if ((Section->CompilePC & 0xFFC) != 0xFFC) {
			switch (BranchType) {
			case BranchTypeRs: EffectDelaySlot = DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0); break;
			case BranchTypeRsRt: EffectDelaySlot = DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,Opcode.rt); break;
			case BranchTypeCop1: 
				{
					OPCODE Command;

					if (!r4300i_LW_VAddr(Section->CompilePC + 4, &Command.Hex)) {
						DisplayError(GS(MSG_FAIL_LOAD_WORD));
						ExitThread(0);
					}
					
					EffectDelaySlot = FALSE;
					if (Command.op == R4300i_CP1) {
						if (Command.fmt == R4300i_COP1_S && (Command.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
						if (Command.fmt == R4300i_COP1_D && (Command.funct & 0x30) == 0x30 ) {
							EffectDelaySlot = TRUE;
						} 
					}
				}
				break;
#ifndef EXTERNAL_RELEASE
			default:
				DisplayError("Unknown branch type");
#endif
			}
		} else {
			EffectDelaySlot = TRUE;
		}
		if (Section->ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((BLOCK_SECTION *)Section->ContinueSection)->SectionID);
		} else {
			strcpy(ContLabel,"Cont.LinkLocationinue");
		}
		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((BLOCK_SECTION *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"Jump.LinkLocation");
		}
		Section->Jump.TargetPC        = Section->CompilePC + ((short)Opcode.offset << 2) + 4;
		Section->Jump.BranchLabel     = JumpLabel;
		Section->Jump.LinkLocation    = NULL;
		Section->Jump.LinkLocation2   = NULL;
		Section->Jump.DoneDelaySlot   = FALSE;
		Section->Cont.TargetPC        = Section->CompilePC + 8;
		Section->Cont.BranchLabel     = ContLabel;
		Section->Cont.LinkLocation    = NULL;
		Section->Cont.LinkLocation2   = NULL;
		Section->Cont.DoneDelaySlot   = FALSE;
		if (Section->Jump.TargetPC < Section->Cont.TargetPC) {
			Section->Cont.FallThrough = FALSE;
			Section->Jump.FallThrough = TRUE;
		} else {
			Section->Cont.FallThrough = TRUE;
			Section->Jump.FallThrough = FALSE;
		}
		if (Link) {
			UnMap_GPR(Section, 31, FALSE);
			MipsRegLo(31) = Section->CompilePC + 8;
			MipsRegState(31) = STATE_CONST_32;
		}
		if (EffectDelaySlot) {
			if (Section->ContinueSection != NULL) {
				sprintf(ContLabel,"Continue",((BLOCK_SECTION *)Section->ContinueSection)->SectionID);
			} else {
				strcpy(ContLabel,"ExitBlock");
			}
			if (Section->JumpSection != NULL) {
				sprintf(JumpLabel,"Jump",((BLOCK_SECTION *)Section->JumpSection)->SectionID);
			} else {
				strcpy(JumpLabel,"ExitBlock");
			}
			CompareFunc(Section); 
			
			if ((Section->CompilePC & 0xFFC) == 0xFFC) {
				GenerateSectionLinkage(Section);
				NextInstruction = END_BLOCK;
				return;
			}
			if (!Section->Jump.FallThrough && !Section->Cont.FallThrough) {
				if (Section->Jump.LinkLocation != NULL) {
					CPU_Message("");
					CPU_Message("      %s:",Section->Jump.BranchLabel);
					SetJump32((DWORD *)Section->Jump.LinkLocation,(DWORD *)RecompPos);
					Section->Jump.LinkLocation = NULL;
					if (Section->Jump.LinkLocation2 != NULL) {
						SetJump32((DWORD *)Section->Jump.LinkLocation2,(DWORD *)RecompPos);
						Section->Jump.LinkLocation2 = NULL;
					}
					Section->Jump.FallThrough = TRUE;
				} else if (Section->Cont.LinkLocation != NULL){
					CPU_Message("");
					CPU_Message("      %s:",Section->Cont.BranchLabel);
					SetJump32((DWORD *)Section->Cont.LinkLocation,(DWORD *)RecompPos);
					Section->Cont.LinkLocation = NULL;
					if (Section->Cont.LinkLocation2 != NULL) {
						SetJump32((DWORD *)Section->Cont.LinkLocation2,(DWORD *)RecompPos);
						Section->Cont.LinkLocation2 = NULL;
					}
					Section->Cont.FallThrough = TRUE;
				}
			}
			for (count = 1; count < 10; count ++) { x86Protected(count) = FALSE; }
			memcpy(&RegBeforeDelay,&Section->RegWorking,sizeof(REG_INFO));
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (EffectDelaySlot) { 
			JUMP_INFO * FallInfo = Section->Jump.FallThrough?&Section->Jump:&Section->Cont;
			JUMP_INFO * JumpInfo = Section->Jump.FallThrough?&Section->Cont:&Section->Jump;

			if (FallInfo->FallThrough && !FallInfo->DoneDelaySlot) {
				if (FallInfo == &Section->Jump) {
					if (Section->JumpSection != NULL) {
						sprintf(JumpLabel,"Section_%d",((BLOCK_SECTION *)Section->JumpSection)->SectionID);
					} else {
						strcpy(JumpLabel,"ExitBlock");
					}
				} else {
					if (Section->ContinueSection != NULL) {
						sprintf(ContLabel,"Section_%d",((BLOCK_SECTION *)Section->ContinueSection)->SectionID);
					} else {
						strcpy(ContLabel,"ExitBlock");
					}
				}
				for (count = 1; count < 10; count ++) { x86Protected(count) = FALSE; }
				memcpy(&FallInfo->RegSet,&Section->RegWorking,sizeof(REG_INFO));
				FallInfo->DoneDelaySlot = TRUE;
				if (!JumpInfo->DoneDelaySlot) {
					FallInfo->FallThrough = FALSE;				
					JmpLabel32(FallInfo->BranchLabel,0);
					FallInfo->LinkLocation = RecompPos - 4;
					
					if (JumpInfo->LinkLocation != NULL) {
						CPU_Message("      %s:",JumpInfo->BranchLabel);
						SetJump32((DWORD *)JumpInfo->LinkLocation,(DWORD *)RecompPos);
						JumpInfo->LinkLocation = NULL;
						if (JumpInfo->LinkLocation2 != NULL) {
							SetJump32((DWORD *)JumpInfo->LinkLocation2,(DWORD *)RecompPos);
							JumpInfo->LinkLocation2 = NULL;
						}
						JumpInfo->FallThrough = TRUE;
						NextInstruction = DO_DELAY_SLOT;
						memcpy(&Section->RegWorking,&RegBeforeDelay,sizeof(REG_INFO));
						return; 
					}
				}
			}
		} else {
			int count;

			CompareFunc(Section); 
			for (count = 1; count < 10; count ++) { x86Protected(count) = FALSE; }
			memcpy(&Section->Cont.RegSet,&Section->RegWorking,sizeof(REG_INFO));
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
		}
		GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_BranchLikely (BLOCK_SECTION * Section, void (*CompareFunc)(BLOCK_SECTION * Section), BOOL Link) {
	static char ContLabel[100], JumpLabel[100];
	int count;

	if ( NextInstruction == NORMAL ) {		
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		
		if (Section->ContinueSection != NULL) {
			sprintf(ContLabel,"Section_%d",((BLOCK_SECTION *)Section->ContinueSection)->SectionID);
		} else {
			strcpy(ContLabel,"ExitBlock");
		}
		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((BLOCK_SECTION *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		Section->Jump.TargetPC      = Section->CompilePC + ((short)Opcode.offset << 2) + 4;
		Section->Jump.BranchLabel   = JumpLabel;
		Section->Jump.FallThrough   = TRUE;
		Section->Jump.LinkLocation  = NULL;
		Section->Jump.LinkLocation2 = NULL;
		Section->Cont.TargetPC      = Section->CompilePC + 8;
		Section->Cont.BranchLabel   = ContLabel;
		Section->Cont.FallThrough   = FALSE;
		Section->Cont.LinkLocation  = NULL;
		Section->Cont.LinkLocation2 = NULL;
		if (Link) {
			UnMap_GPR(Section, 31, FALSE);
			MipsRegLo(31) = Section->CompilePC + 8;
			MipsRegState(31) = STATE_CONST_32;
		}
		CompareFunc(Section); 
		for (count = 1; count < 10; count ++) { x86Protected(count) = FALSE; }
		memcpy(&Section->Cont.RegSet,&Section->RegWorking,sizeof(REG_INFO));
	
		if (Section->Cont.FallThrough)  {
			if (Section->Jump.LinkLocation != NULL) {
#ifndef EXTERNAL_RELEASE
				DisplayError("WTF .. problem with Compile_R4300i_BranchLikely");
#endif
			}
			GenerateSectionLinkage(Section);
			NextInstruction = END_BLOCK;
		} else {
			if ((Section->CompilePC & 0xFFC) == 0xFFC) {
				Section->Jump.FallThrough = FALSE;
				if (Section->Jump.LinkLocation != NULL) {
					SetJump32(Section->Jump.LinkLocation,RecompPos);
					Section->Jump.LinkLocation = NULL;
					if (Section->Jump.LinkLocation2 != NULL) { 
						SetJump32(Section->Jump.LinkLocation2,RecompPos);
						Section->Jump.LinkLocation2 = NULL;
					}
				}
				JmpLabel32("DoDelaySlot",0);
				Section->Jump.LinkLocation = RecompPos - 4;
				CPU_Message("      ");
				CPU_Message("      %s:",Section->Cont.BranchLabel);
				if (Section->Cont.LinkLocation != NULL) {
					SetJump32(Section->Cont.LinkLocation,RecompPos);
					Section->Cont.LinkLocation = NULL;
					if (Section->Cont.LinkLocation2 != NULL) { 
						SetJump32(Section->Cont.LinkLocation2,RecompPos);
						Section->Cont.LinkLocation2 = NULL;
					}
				}
				CompileExit (Section->CompilePC + 8,Section->RegWorking,Normal,TRUE,NULL);
				CPU_Message("      ");
				CPU_Message("      DoDelaySlot");
				GenerateSectionLinkage(Section);
				NextInstruction = END_BLOCK;
			} else {
				NextInstruction = DO_DELAY_SLOT;
			}
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		for (count = 1; count < 10; count ++) { x86Protected(count) = FALSE; }
		memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
		GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranchLikely\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void BNE_Compare (BLOCK_SECTION * Section) {
	BYTE *Jump;

	if (IsKnown(Opcode.rs) && IsKnown(Opcode.rt)) {
		if (IsConst(Opcode.rs) && IsConst(Opcode.rt)) {
			if (Is64Bit(Opcode.rs) || Is64Bit(Opcode.rt)) {
				Compile_R4300i_UnknownOpcode(Section);
			} else if (MipsRegLo(Opcode.rs) != MipsRegLo(Opcode.rt)) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (IsMapped(Opcode.rs) && IsMapped(Opcode.rt)) {
			if (Is64Bit(Opcode.rs) || Is64Bit(Opcode.rt)) {
				ProtectGPR(Section,Opcode.rs);
				ProtectGPR(Section,Opcode.rt);

				CompX86RegToX86Reg(
					Is32Bit(Opcode.rs)?Map_TempReg(Section,x86_Any,Opcode.rs,TRUE):MipsRegHi(Opcode.rs),
					Is32Bit(Opcode.rt)?Map_TempReg(Section,x86_Any,Opcode.rt,TRUE):MipsRegHi(Opcode.rt)
				);
					
				if (Section->Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs),MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation2 = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation2 = RecompPos - 4;
				}
			} else {
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs),MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		} else {
			DWORD ConstReg = IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Is64Bit(ConstReg) || Is64Bit(MappedReg)) {
				if (Is32Bit(ConstReg) || Is32Bit(MappedReg)) {
					ProtectGPR(Section,MappedReg);
					if (Is32Bit(MappedReg)) {
						CompConstToX86reg(Map_TempReg(Section,x86_Any,MappedReg,TRUE),MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(MipsRegHi(MappedReg),(int)MipsRegLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(MipsRegHi(MappedReg),MipsRegHi(ConstReg));
				}
				if (Section->Jump.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
				if (MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(MipsRegLo(MappedReg),MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(MipsRegLo(MappedReg),MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation2 = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation2 = RecompPos - 4;
				}
			} else {
				if (MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(MipsRegLo(MappedReg),MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(MipsRegLo(MappedReg),MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JneLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JeLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		}
	} else if (IsKnown(Opcode.rs) || IsKnown(Opcode.rt)) {
		DWORD KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else if (IsSigned(KnownReg)) {
				CompConstToVariable(((int)MipsRegLo(KnownReg) >> 31),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				CompConstToVariable(0,&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}
		} else {
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else if (IsSigned(KnownReg)) {
				ProtectGPR(Section,KnownReg);
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				CompConstToVariable(0,&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}
		}
		if (Section->Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
		if (IsConst(KnownReg)) {
			CompConstToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		} else {
			CompX86regToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		}
		if (Section->Cont.FallThrough) {
			JneLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation2 = RecompPos - 4;
		} else if (Section->Jump.FallThrough) {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation2 = RecompPos - 4;
		}
	} else {
		int x86Reg;

		x86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);		
		CompX86regToVariable(x86Reg,&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs]);
		if (Section->Jump.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}

		x86Reg = Map_TempReg(Section,x86Reg,Opcode.rt,FALSE);
		CompX86regToVariable(x86Reg,&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs]);
		if (Section->Cont.FallThrough) {
			JneLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation2 = RecompPos - 4;
		} else if (Section->Jump.FallThrough) {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else {
			JeLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation2 = RecompPos - 4;
		}
	}
}

void BEQ_Compare (BLOCK_SECTION * Section) {
	BYTE *Jump;

	if (IsKnown(Opcode.rs) && IsKnown(Opcode.rt)) {
		if (IsConst(Opcode.rs) && IsConst(Opcode.rt)) {
			if (Is64Bit(Opcode.rs) || Is64Bit(Opcode.rt)) {
				Compile_R4300i_UnknownOpcode(Section);
			} else if (MipsRegLo(Opcode.rs) == MipsRegLo(Opcode.rt)) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (IsMapped(Opcode.rs) && IsMapped(Opcode.rt)) {
			if (Is64Bit(Opcode.rs) || Is64Bit(Opcode.rt)) {
				ProtectGPR(Section,Opcode.rs);
				ProtectGPR(Section,Opcode.rt);

				CompX86RegToX86Reg(
					Is32Bit(Opcode.rs)?Map_TempReg(Section,x86_Any,Opcode.rs,TRUE):MipsRegHi(Opcode.rs),
					Is32Bit(Opcode.rt)?Map_TempReg(Section,x86_Any,Opcode.rt,TRUE):MipsRegHi(Opcode.rt)
				);
				if (Section->Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Cont.BranchLabel,0);
					Section->Cont.LinkLocation = RecompPos - 4;
				}
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs),MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			} else {
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs),MipsRegLo(Opcode.rt));
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		} else {
			DWORD ConstReg = IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Is64Bit(ConstReg) || Is64Bit(MappedReg)) {
				if (Is32Bit(ConstReg) || Is32Bit(MappedReg)) {
					if (Is32Bit(MappedReg)) {
						ProtectGPR(Section,MappedReg);
						CompConstToX86reg(Map_TempReg(Section,x86_Any,MappedReg,TRUE),MipsRegHi(ConstReg));
					} else {
						CompConstToX86reg(MipsRegHi(MappedReg),(int)MipsRegLo(ConstReg) >> 31);
					}
				} else {
					CompConstToX86reg(MipsRegHi(MappedReg),MipsRegHi(ConstReg));
				}			
				if (Section->Cont.FallThrough) {
					JneLabel8("continue",0);
					Jump = RecompPos - 1;
				} else {
					JneLabel32(Section->Cont.BranchLabel,0);
					Section->Cont.LinkLocation = RecompPos - 4;
				}
				if (MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(MipsRegLo(MappedReg),MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(MipsRegLo(MappedReg),MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
					CPU_Message("      ");
					CPU_Message("      continue:");
					SetJump8(Jump,RecompPos);
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation2 = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			} else {
				if (MipsRegLo(ConstReg) == 0) {
					OrX86RegToX86Reg(MipsRegLo(MappedReg),MipsRegLo(MappedReg));
				} else {
					CompConstToX86reg(MipsRegLo(MappedReg),MipsRegLo(ConstReg));
				}
				if (Section->Cont.FallThrough) {
					JeLabel32 ( Section->Jump.BranchLabel, 0 );
					Section->Jump.LinkLocation = RecompPos - 4;
				} else if (Section->Jump.FallThrough) {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
				} else {
					JneLabel32 ( Section->Cont.BranchLabel, 0 );
					Section->Cont.LinkLocation = RecompPos - 4;
					JmpLabel32(Section->Jump.BranchLabel,0);
					Section->Jump.LinkLocation = RecompPos - 4;
				}
			}
		}
	} else if (IsKnown(Opcode.rs) || IsKnown(Opcode.rt)) {
		DWORD KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else if (IsSigned(KnownReg)) {
				CompConstToVariable((int)MipsRegLo(KnownReg) >> 31,&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				CompConstToVariable(0,&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}
		} else {
			ProtectGPR(Section,KnownReg);
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else if (IsSigned(KnownReg)) {
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				CompConstToVariable(0,&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}
		}
		if (Section->Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Cont.BranchLabel,0);
			Section->Cont.LinkLocation = RecompPos - 4;
		}
		if (IsConst(KnownReg)) {
			CompConstToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		} else {
			CompX86regToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		}
		if (Section->Cont.FallThrough) {
			JeLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else if (Section->Jump.FallThrough) {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
		} else {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	} else {
		int x86Reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&GPR[Opcode.rt].W[1],GPR_NameHi[Opcode.rt]);
		if (Section->Cont.FallThrough) {
			JneLabel8("continue",0);
			Jump = RecompPos - 1;
		} else {
			JneLabel32(Section->Cont.BranchLabel,0);
			Section->Cont.LinkLocation = RecompPos - 4;
		}
		CompX86regToVariable(Map_TempReg(Section,x86Reg,Opcode.rs,FALSE),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
		if (Section->Cont.FallThrough) {
			JeLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      ");
			CPU_Message("      continue:");
			SetJump8(Jump,RecompPos);
		} else if (Section->Jump.FallThrough) {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
		} else {
			JneLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	}
}

void BGTZ_Compare (BLOCK_SECTION * Section) {
	if (IsConst(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			if (MipsReg_S(Opcode.rs) > 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			if (MipsRegLo_S(Opcode.rs) > 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		}
	} else if (IsMapped(Opcode.rs) && Is32Bit(Opcode.rs)) {
		CompConstToX86reg(MipsRegLo(Opcode.rs),0);
		if (Section->Jump.FallThrough) {
			JleLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		} else if (Section->Cont.FallThrough) {
			JgLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JleLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	} else {
		BYTE *Jump;

		if (IsMapped(Opcode.rs)) {
			CompConstToX86reg(MipsRegHi(Opcode.rs),0);
		} else {
			CompConstToVariable(0,&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs]);
		}
		if (Section->Jump.FallThrough) {
			JlLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JgLabel8("continue",0);
			Jump = RecompPos - 1;
		} else if (Section->Cont.FallThrough) {
			JlLabel8("continue",0);
			Jump = RecompPos - 1;
			JgLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JgLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}

		if (IsMapped(Opcode.rs)) {
			CompConstToX86reg(MipsRegLo(Opcode.rs),0);
		} else {
			CompConstToVariable(0,&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs]);
		}
		if (Section->Jump.FallThrough) {
			JeLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		} else if (Section->Cont.FallThrough) {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
			CPU_Message("      continue:");
			*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		} else {
			JneLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
			JmpLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation2 = RecompPos - 4;
		}
	}
}

void BLEZ_Compare (BLOCK_SECTION * Section) {
	if (IsConst(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			if (MipsReg_S(Opcode.rs) <= 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (IsSigned(Opcode.rs)) {
			if (MipsRegLo_S(Opcode.rs) <= 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			if (MipsRegLo(Opcode.rs) == 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		}
	} else {
		if (IsMapped(Opcode.rs) && Is32Bit(Opcode.rs)) {
			CompConstToX86reg(MipsRegLo(Opcode.rs),0);
			if (Section->Jump.FallThrough) {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else if (Section->Cont.FallThrough) {
				JleLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else {
			BYTE *Jump;

			if (IsMapped(Opcode.rs)) {
				CompConstToX86reg(MipsRegHi(Opcode.rs),0);
			} else {
				CompConstToVariable(0,&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs]);
			}
			if (Section->Jump.FallThrough) {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JlLabel8("Continue",0);
				Jump = RecompPos - 1;
			} else if (Section->Cont.FallThrough) {
				JgLabel8("Continue",0);
				Jump = RecompPos - 1;
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}

			if (IsMapped(Opcode.rs)) {
				CompConstToX86reg(MipsRegLo(Opcode.rs),0);
			} else {
				CompConstToVariable(0,&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs]);
			}
			if (Section->Jump.FallThrough) {
				JneLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation2 = RecompPos - 4;
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			} else if (Section->Cont.FallThrough) {
				JeLabel32 (Section->Jump.BranchLabel, 0 );
				Section->Jump.LinkLocation2 = RecompPos - 4;
				CPU_Message("      continue:");
				*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
			} else {
				JneLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation2 = RecompPos - 4;
				JmpLabel32("BranchToJump",0);
				Section->Jump.LinkLocation2 = RecompPos - 4;
			}
		}
	}
}

void BLTZ_Compare (BLOCK_SECTION * Section) {
	if (IsConst(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			if (MipsReg_S(Opcode.rs) < 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else if (IsSigned(Opcode.rs)) {
			if (MipsRegLo_S(Opcode.rs) < 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			Section->Jump.FallThrough = FALSE;
			Section->Cont.FallThrough = TRUE;
		}
	} else if (IsMapped(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			CompConstToX86reg(MipsRegHi(Opcode.rs),0);
			if (Section->Jump.FallThrough) {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else if (Section->Cont.FallThrough) {
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else if (IsSigned(Opcode.rs)) {
			CompConstToX86reg(MipsRegLo(Opcode.rs),0);
			if (Section->Jump.FallThrough) {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else if (Section->Cont.FallThrough) {
				JlLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			} else {
				JgeLabel32 (Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else {
			Section->Jump.FallThrough = FALSE;
			Section->Cont.FallThrough = TRUE;
		}
	} else if (IsUnknown(Opcode.rs)) {
		CompConstToVariable(0,&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs]);
		if (Section->Jump.FallThrough) {
			JgeLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		} else if (Section->Cont.FallThrough) {
			JlLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
			JmpLabel32 (Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		}
	}
}

void BGEZ_Compare (BLOCK_SECTION * Section) {
	if (IsConst(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
			DisplayError("BGEZ 1");
#endif
			Compile_R4300i_UnknownOpcode(Section);
		} else if IsSigned(Opcode.rs) {
			if (MipsRegLo_S(Opcode.rs) >= 0) {
				Section->Jump.FallThrough = TRUE;
				Section->Cont.FallThrough = FALSE;
			} else {
				Section->Jump.FallThrough = FALSE;
				Section->Cont.FallThrough = TRUE;
			}
		} else {
			Section->Jump.FallThrough = TRUE;
			Section->Cont.FallThrough = FALSE;
		}
	} else if (IsMapped(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) { 
			CompConstToX86reg(MipsRegHi(Opcode.rs),0);
			if (Section->Cont.FallThrough) {
				JgeLabel32 ( Section->Jump.BranchLabel, 0 );
				Section->Jump.LinkLocation = RecompPos - 4;
			} else if (Section->Jump.FallThrough) {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else if (IsSigned(Opcode.rs)) {
			CompConstToX86reg(MipsRegLo(Opcode.rs),0);
			if (Section->Cont.FallThrough) {
				JgeLabel32 ( Section->Jump.BranchLabel, 0 );
				Section->Jump.LinkLocation = RecompPos - 4;
			} else if (Section->Jump.FallThrough) {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
			} else {
				JlLabel32 ( Section->Cont.BranchLabel, 0 );
				Section->Cont.LinkLocation = RecompPos - 4;
				JmpLabel32(Section->Jump.BranchLabel,0);
				Section->Jump.LinkLocation = RecompPos - 4;
			}
		} else { 
			Section->Jump.FallThrough = TRUE;
			Section->Cont.FallThrough = FALSE;
		}
	} else {
		CompConstToVariable(0,&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs]);		
		if (Section->Cont.FallThrough) {
			JgeLabel32 ( Section->Jump.BranchLabel, 0 );
			Section->Jump.LinkLocation = RecompPos - 4;
		} else if (Section->Jump.FallThrough) {
			JlLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
		} else {
			JlLabel32 ( Section->Cont.BranchLabel, 0 );
			Section->Cont.LinkLocation = RecompPos - 4;
			JmpLabel32(Section->Jump.BranchLabel,0);
			Section->Jump.LinkLocation = RecompPos - 4;
		}
	}
}

void COP1_BCF_Compare (BLOCK_SECTION * Section) {
	TestVariable(FPCSR_C,&FPCR[31],"FPCR[31]");
	if (Section->Cont.FallThrough) {
		JeLabel32 ( Section->Jump.BranchLabel, 0 );
		Section->Jump.LinkLocation = RecompPos - 4;
	} else if (Section->Jump.FallThrough) {
		JneLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
	} else {
		JneLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
		JmpLabel32(Section->Jump.BranchLabel,0);
		Section->Jump.LinkLocation = RecompPos - 4;
	}
}

void COP1_BCT_Compare (BLOCK_SECTION * Section) {
	TestVariable(FPCSR_C,&FPCR[31],"FPCR[31]");
	if (Section->Cont.FallThrough) {
		JneLabel32 ( Section->Jump.BranchLabel, 0 );
		Section->Jump.LinkLocation = RecompPos - 4;
	} else if (Section->Jump.FallThrough) {
		JeLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
	} else {
		JeLabel32 ( Section->Cont.BranchLabel, 0 );
		Section->Cont.LinkLocation = RecompPos - 4;
		JmpLabel32(Section->Jump.BranchLabel,0);
		Section->Jump.LinkLocation = RecompPos - 4;
	}
}
/*************************  OpCode functions *************************/
void Compile_R4300i_J (BLOCK_SECTION * Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((BLOCK_SECTION *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		Section->Jump.TargetPC      = (Section->CompilePC & 0xF0000000) + (Opcode.target << 2);;
		Section->Jump.BranchLabel   = JumpLabel;
		Section->Jump.FallThrough   = TRUE;
		Section->Jump.LinkLocation  = NULL;
		Section->Jump.LinkLocation2 = NULL;
		NextInstruction = DO_DELAY_SLOT;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
			GenerateSectionLinkage(Section);
			NextInstruction = END_BLOCK;
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
		GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_JAL (BLOCK_SECTION * Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		UnMap_GPR(Section, 31, FALSE);
		MipsRegLo(31) = Section->CompilePC + 8;
		MipsRegState(31) = STATE_CONST_32;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			MoveConstToVariable((Section->CompilePC & 0xF0000000) + (Opcode.target << 2),&JumpToLocation,"JumpToLocation");
			MoveConstToVariable(Section->CompilePC + 4,&PROGRAM_COUNTER,"PROGRAM_COUNTER");
			if (BlockCycleCount != 0) { 
				AddConstToVariable(BlockCycleCount,&CP0[9],Cop0_Name[9]); 
				SubConstFromVariable(BlockCycleCount,&Timers.Timer,"Timer");
			}
			if (BlockRandomModifier != 0) { SubConstFromVariable(BlockRandomModifier,&CP0[1],Cop0_Name[1]); }
			WriteBackRegisters(Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		MoveConstToVariable((Section->CompilePC & 0xF0000000) + (Opcode.target << 2),&PROGRAM_COUNTER,"PROGRAM_COUNTER");
		CompileExit((DWORD)-1,Section->RegWorking,Normal,TRUE,NULL);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
	return;

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

		UnMap_GPR(Section, 31, FALSE);
		MipsRegLo(31) = Section->CompilePC + 8;
		MipsRegState(31) = STATE_CONST_32;
		NextInstruction = DO_DELAY_SLOT;
		if (Section->JumpSection != NULL) {
			sprintf(JumpLabel,"Section_%d",((BLOCK_SECTION *)Section->JumpSection)->SectionID);
		} else {
			strcpy(JumpLabel,"ExitBlock");
		}
		Section->Jump.TargetPC      = (Section->CompilePC & 0xF0000000) + (Opcode.target << 2);
		Section->Jump.BranchLabel   = JumpLabel;
		Section->Jump.FallThrough   = TRUE;
		Section->Jump.LinkLocation  = NULL;
		Section->Jump.LinkLocation2 = NULL;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
			GenerateSectionLinkage(Section);
			NextInstruction = END_BLOCK;
		}
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
		GenerateSectionLinkage(Section);
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nJal\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_ADDI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) { return; }

	if (SPHack && Opcode.rs == 29 && Opcode.rt == 29) {
		AddConstToX86Reg(Map_MemoryStack(Section, TRUE),(short)Opcode.immediate);
	}

	if (IsConst(Opcode.rs)) { 
		if (IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		MipsRegLo(Opcode.rt) = MipsRegLo(Opcode.rs) + (short)Opcode.immediate;
		MipsRegState(Opcode.rt) = STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rs);
	if (Opcode.immediate == 0) { 
	} else if (Opcode.immediate == 1) {
		IncX86reg(MipsRegLo(Opcode.rt));
	} else if (Opcode.immediate == 0xFFFF) {			
		DecX86reg(MipsRegLo(Opcode.rt));
	} else {
		AddConstToX86Reg(MipsRegLo(Opcode.rt),(short)Opcode.immediate);
	}
	if (SPHack && Opcode.rt == 29 && Opcode.rs != 29) { 
		int count;

		for (count = 0; count < 10; count ++) { x86Protected(count) = FALSE; }
		ResetMemoryStack(Section); 
	}

}

void Compile_R4300i_ADDIU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) { return; }

	if (SPHack && Opcode.rs == 29 && Opcode.rt == 29) {
		AddConstToX86Reg(Map_MemoryStack(Section, TRUE),(short)Opcode.immediate);
	}

	if (IsConst(Opcode.rs)) { 
		if (IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		MipsRegLo(Opcode.rt) = MipsRegLo(Opcode.rs) + (short)Opcode.immediate;
		MipsRegState(Opcode.rt) = STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rs);
	if (Opcode.immediate == 0) { 
	} else if (Opcode.immediate == 1) {
		IncX86reg(MipsRegLo(Opcode.rt));
	} else if (Opcode.immediate == 0xFFFF) {			
		DecX86reg(MipsRegLo(Opcode.rt));
	} else {
		AddConstToX86Reg(MipsRegLo(Opcode.rt),(short)Opcode.immediate);
	}
	if (SPHack && Opcode.rt == 29 && Opcode.rs != 29) { 
		int count;

		for (count = 0; count < 10; count ++) { x86Protected(count) = FALSE; }
		ResetMemoryStack(Section); 
	}
}

void Compile_R4300i_SLTIU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return; }

	if (IsConst(Opcode.rs)) { 
		DWORD Result;

		if (Is64Bit(Opcode.rs)) {
			_int64 Immediate = (_int64)((short)Opcode.immediate);
			Result = MipsReg(Opcode.rs) < ((unsigned)(Immediate))?1:0;
		} else if (Is32Bit(Opcode.rs)) {
			Result = MipsRegLo(Opcode.rs) < ((unsigned)((short)Opcode.immediate))?1:0;
		}
		UnMap_GPR(Section,Opcode.rt, FALSE);
		MipsRegState(Opcode.rt) = STATE_CONST_32;
		MipsRegLo(Opcode.rt) = Result;
	} else if (IsMapped(Opcode.rs)) { 
		if (Is64Bit(Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(MipsRegHi(Opcode.rs),((short)Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = RecompPos - 1;
			SetbVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
			CompConstToX86reg(MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));
		} else {
			CompConstToX86reg(MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));
		}
	} else {
		BYTE * Jump;

		CompConstToVariable(((short)Opcode.immediate >> 31),&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs]);
		JneLabel8("CompareSet",0);
		Jump = RecompPos - 1;
		CompConstToVariable((short)Opcode.immediate,&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs]);
		CPU_Message("");
		CPU_Message("      CompareSet:");
		*((BYTE *)(Jump))=(BYTE)(RecompPos - Jump - 1);
		SetbVariable(&BranchCompare,"BranchCompare");
		Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));
		
		
		/*SetbVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompConstToVariable((short)Opcode.immediate,&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));*/
	}
}

void Compile_R4300i_SLTI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return; }

	if (IsConst(Opcode.rs)) { 
		DWORD Result;

		if (Is64Bit(Opcode.rs)) {
			_int64 Immediate = (_int64)((short)Opcode.immediate);
			Result = (_int64)MipsReg(Opcode.rs) < Immediate?1:0;
		} else if (Is32Bit(Opcode.rs)) {
			Result = MipsRegLo_S(Opcode.rs) < (short)Opcode.immediate?1:0;
		}
		UnMap_GPR(Section,Opcode.rt, FALSE);
		MipsRegState(Opcode.rt) = STATE_CONST_32;
		MipsRegLo(Opcode.rt) = Result;
	} else if (IsMapped(Opcode.rs)) { 
		if (Is64Bit(Opcode.rs)) {
			BYTE * Jump[2];

			CompConstToX86reg(MipsRegHi(Opcode.rs),((short)Opcode.immediate >> 31));
			JeLabel8("Low Compare",0);
			Jump[0] = RecompPos - 1;
			SetlVariable(&BranchCompare,"BranchCompare");
			JmpLabel8("Continue",0);
			Jump[1] = RecompPos - 1;
			CPU_Message("");
			CPU_Message("      Low Compare:");
			*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
			CompConstToX86reg(MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetbVariable(&BranchCompare,"BranchCompare");
			CPU_Message("");
			CPU_Message("      Continue:");
			*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));
		} else {
		/*	CompConstToX86reg(MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			SetlVariable(&BranchCompare,"BranchCompare");
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));
			*/
			ProtectGPR(Section, Opcode.rs);
			Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
			CompConstToX86reg(MipsRegLo(Opcode.rs),(short)Opcode.immediate);
			
			if (MipsRegLo(Opcode.rt) > x86_EDX) {
				SetlVariable(&BranchCompare,"BranchCompare");
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));
			} else {
				Setl(MipsRegLo(Opcode.rt));
				AndConstToX86Reg(MipsRegLo(Opcode.rt), 1);
			}
		}
	} else {
		BYTE * Jump[2];

		CompConstToVariable(((short)Opcode.immediate >> 31),&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetlVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompConstToVariable((short)Opcode.immediate,&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rt,FALSE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rt));
	}
}

void Compile_R4300i_ANDI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) { return;}

	if (IsConst(Opcode.rs)) {
		if (IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		MipsRegState(Opcode.rt) = STATE_CONST_32;
		MipsRegLo(Opcode.rt) = MipsRegLo(Opcode.rs) & Opcode.immediate;
	} else if (Opcode.immediate != 0) { 
		Map_GPR_32bit(Section,Opcode.rt,FALSE,Opcode.rs);
		AndConstToX86Reg(MipsRegLo(Opcode.rt),Opcode.immediate);
	} else {
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
	}
}

void Compile_R4300i_ORI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	if (IsConst(Opcode.rs)) {
		if (IsMapped(Opcode.rt)) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		MipsRegState(Opcode.rt) = MipsRegState(Opcode.rs);
		MipsRegHi(Opcode.rt) = MipsRegHi(Opcode.rs);
		MipsRegLo(Opcode.rt) = MipsRegLo(Opcode.rs) | Opcode.immediate;
	} else if (IsMapped(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			Map_GPR_64bit(Section,Opcode.rt,Opcode.rs);
		} else {
			Map_GPR_32bit(Section,Opcode.rt,IsSigned(Opcode.rs),Opcode.rs);
		}
		OrConstToX86Reg(Opcode.immediate,MipsRegLo(Opcode.rt));
	} else {
		Map_GPR_64bit(Section,Opcode.rt,Opcode.rs);
		OrConstToX86Reg(Opcode.immediate,MipsRegLo(Opcode.rt));
	}
}

void Compile_R4300i_XORI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	if (IsConst(Opcode.rs)) {
		if (Opcode.rs != Opcode.rt) { UnMap_GPR(Section,Opcode.rt, FALSE); }
		MipsRegState(Opcode.rt) = MipsRegState(Opcode.rs);
		MipsRegHi(Opcode.rt) = MipsRegHi(Opcode.rs);
		MipsRegLo(Opcode.rt) = MipsRegLo(Opcode.rs) ^ Opcode.immediate;
	} else {
		if (IsMapped(Opcode.rs) && Is32Bit(Opcode.rs)) {
			Map_GPR_32bit(Section,Opcode.rt,IsSigned(Opcode.rs),Opcode.rs);
		} else {
			Map_GPR_64bit(Section,Opcode.rt,Opcode.rs);
		}
		if (Opcode.immediate != 0) { XorConstToX86Reg(MipsRegLo(Opcode.rt),Opcode.immediate); }
	}
}

void Compile_R4300i_LUI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rt == 0) { return;}

	if (SPHack && Opcode.rt == 29) {
		DWORD Address = ((short)Opcode.offset << 16);
		int x86reg = Map_MemoryStack(Section, FALSE);
		TranslateVaddr (&Address);
		if (x86reg < 0) {
			MoveConstToVariable((DWORD)(Address + N64MEM), &MemoryStack, "MemoryStack");
		} else {
			MoveConstToX86reg((DWORD)(Address + N64MEM), x86reg);
		}
	}
	UnMap_GPR(Section,Opcode.rt, FALSE);
	MipsRegLo(Opcode.rt) = ((short)Opcode.offset << 16);
	MipsRegState(Opcode.rt) = STATE_CONST_32;
}

void Compile_R4300i_DADDIU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rs,TRUE); }
	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_DADDIU, "r4300i_DADDIU");
	Popad();
}

void Compile_R4300i_LDL (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_LDL, "r4300i_LDL");
	Popad();

}

void Compile_R4300i_LDR (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_LDR, "r4300i_LDR");
	Popad();
}


void Compile_R4300i_LB (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(Section,Opcode.rt,TRUE,0);
		Compile_LB(MipsRegLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxByteX86regPointerToX86reg(TempReg1, TempReg2,MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regByte(MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LH (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(Section,Opcode.rt,TRUE,0);
		Compile_LH(MipsRegLo(Opcode.rt),Address,TRUE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxHalfX86regPointerToX86reg(TempReg1, TempReg2,MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveSxN64MemToX86regHalf(MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LWL (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address, Value;
		
		Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(MipsRegLo(Opcode.rt),LWL_MASK[Offset]);
		ShiftLeftSignImmed(Value,(BYTE)LWL_SHIFT[Offset]);
		AddX86RegToX86Reg(MipsRegLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
	}
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg(LWL_MASK,"LWL_MASK",MipsRegLo(Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWL_SHIFT,"LWL_SHIFT",shift,Offset,4);
	if (UseTlb) {			
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftLeftSign(TempReg1);
	AddX86RegToX86Reg(MipsRegLo(Opcode.rt),TempReg1);
}

void Compile_R4300i_LW (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (Opcode.base == 29 && SPHack) {
		char String[100];

		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		TempReg1 = Map_MemoryStack(Section,TRUE);
		sprintf(String,"%Xh",(short)Opcode.offset);
		MoveVariableDispToX86Reg((void *)((DWORD)(short)Opcode.offset),String,MipsRegLo(Opcode.rt),TempReg1,1);
	} else {
		if (IsConst(Opcode.base)) { 
			DWORD Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
			Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
			Compile_LW(MipsRegLo(Opcode.rt),Address);
			return;
		}
		if (UseTlb) {	
			if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
			if (IsMapped(Opcode.base) && Opcode.offset == 0) { 
				ProtectGPR(Section,Opcode.base);
				TempReg1 = MipsRegLo(Opcode.base);
			} else {
				if (IsMapped(Opcode.base)) { 
					ProtectGPR(Section,Opcode.base);
					if (Opcode.offset != 0) {
						TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
						LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
					} else {
						TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
					}
				} else {
					TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
					if (Opcode.immediate == 0) { 
					} else if (Opcode.immediate == 1) {
						IncX86reg(TempReg1);
					} else if (Opcode.immediate == 0xFFFF) {			
						DecX86reg(TempReg1);
					} else {
						AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
					}
				}
			}
			TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
			MoveX86RegToX86Reg(TempReg1, TempReg2);
			ShiftRightUnsignImmed(TempReg2,12);
			MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
			CompileReadTLBMiss(Section,TempReg1,TempReg2);
			Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
			MoveX86regPointerToX86reg(TempReg1, TempReg2,MipsRegLo(Opcode.rt));
		} else {
			if (IsMapped(Opcode.base)) { 
				ProtectGPR(Section,Opcode.base);
				if (Opcode.offset != 0) {
					Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
					LeaSourceAndOffset(MipsRegLo(Opcode.rt),MipsRegLo(Opcode.base),(short)Opcode.offset);
				} else {
					Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
				}
			} else {
				Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
				if (Opcode.immediate == 0) { 
				} else if (Opcode.immediate == 1) {
					IncX86reg(MipsRegLo(Opcode.rt));
				} else if (Opcode.immediate == 0xFFFF) {			
					DecX86reg(MipsRegLo(Opcode.rt));
				} else {
					AddConstToX86Reg(MipsRegLo(Opcode.rt),(short)Opcode.immediate);
				}
			}
			AndConstToX86Reg(MipsRegLo(Opcode.rt),0x1FFFFFFF);
			MoveN64MemToX86reg(MipsRegLo(Opcode.rt),MipsRegLo(Opcode.rt));
		}
	}
}

void Compile_R4300i_LBU (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
		Compile_LB(MipsRegLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,3);	
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxByteX86regPointerToX86reg(TempReg1, TempReg2,MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxN64MemToX86regByte(MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LHU (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
		Compile_LH(MipsRegLo(Opcode.rt),Address,FALSE);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		XorConstToX86Reg(TempReg1,2);	
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,2);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_LWR (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address, Value;
		
		Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;

		Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(MipsRegLo(Opcode.rt),LWR_MASK[Offset]);
		ShiftRightUnsignImmed(Value,(BYTE)LWR_SHIFT[Offset]);
		AddX86RegToX86Reg(MipsRegLo(Opcode.rt),Value);
		return;
	}

	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
	}
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.rt);
	AndVariableDispToX86Reg(LWR_MASK,"LWR_MASK",MipsRegLo(Opcode.rt),Offset,4);
	MoveVariableDispToX86Reg(LWR_SHIFT,"LWR_SHIFT",shift,Offset,4);
	if (UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,TempReg1);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(TempReg1,TempReg1);
	}
	ShiftRightUnsign(TempReg1);
	AddX86RegToX86Reg(MipsRegLo(Opcode.rt),TempReg1);
}

void Compile_R4300i_LWU (BLOCK_SECTION * Section) {			//added by Witten
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = (MipsRegLo(Opcode.base) + (short)Opcode.offset);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,0);
		Compile_LW(MipsRegLo(Opcode.rt),Address);
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveZxHalfX86regPointerToX86reg(TempReg1, TempReg2,MipsRegLo(Opcode.rt));
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveZxN64MemToX86regHalf(MipsRegLo(Opcode.rt), TempReg1);
	}
}

void Compile_R4300i_SB (BLOCK_SECTION * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (IsConst(Opcode.base)) { 
		DWORD Address = (MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 3;
		
		if (IsConst(Opcode.rt)) {
			Compile_SB_Const((BYTE)MipsRegLo(Opcode.rt), Address);
		} else if (IsMapped(Opcode.rt) && Is8BitReg(MipsRegLo(Opcode.rt))) {
			Compile_SB_Register(MipsRegLo(Opcode.rt), Address);
		} else {
			Compile_SB_Register(Map_TempReg(Section,x86_Any8Bit,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		CompileWriteTLBMiss(Section, TempReg1, TempReg2);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,3);	
		if (IsConst(Opcode.rt)) {
			MoveConstByteToX86regPointer((BYTE)MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(Opcode.rt) && Is8BitReg(MipsRegLo(Opcode.rt))) {
			MoveX86regByteToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			UnProtectGPR(Section,Opcode.rt);
			MoveX86regByteToX86regPointer(Map_TempReg(Section,x86_Any8Bit,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		XorConstToX86Reg(TempReg1,3);
		if (IsConst(Opcode.rt)) {
			MoveConstByteToN64Mem((BYTE)MipsRegLo(Opcode.rt),TempReg1);
		} else if (IsMapped(Opcode.rt) && Is8BitReg(MipsRegLo(Opcode.rt))) {
			MoveX86regByteToN64Mem(MipsRegLo(Opcode.rt),TempReg1);
		} else {	
			UnProtectGPR(Section,Opcode.rt);
			MoveX86regByteToN64Mem(Map_TempReg(Section,x86_Any8Bit,Opcode.rt,FALSE),TempReg1);
		}
	}

}

void Compile_R4300i_SH (BLOCK_SECTION * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (IsConst(Opcode.base)) { 
		DWORD Address = (MipsRegLo(Opcode.base) + (short)Opcode.offset) ^ 2;
		
		if (IsConst(Opcode.rt)) {
			Compile_SH_Const((WORD)MipsRegLo(Opcode.rt), Address);
		} else if (IsMapped(Opcode.rt)) {
			Compile_SH_Register(MipsRegLo(Opcode.rt), Address);
		} else {
			Compile_SH_Register(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), Address);
		}
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		CompileWriteTLBMiss(Section, TempReg1, TempReg2);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		XorConstToX86Reg(TempReg1,2);	
		if (IsConst(Opcode.rt)) {
			MoveConstHalfToX86regPointer((WORD)MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regHalfToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regHalfToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		XorConstToX86Reg(TempReg1,2);
		if (IsConst(Opcode.rt)) {
			MoveConstHalfToN64Mem((WORD)MipsRegLo(Opcode.rt),TempReg1);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regHalfToN64Mem(MipsRegLo(Opcode.rt),TempReg1);		
		} else {	
			MoveX86regHalfToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1);		
		}
	}
}

void Compile_R4300i_SWL (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (IsConst(Opcode.base)) { 
		DWORD Address;
	
		Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;
		
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(Value,SWL_MASK[Offset]);
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
		ShiftRightUnsignImmed(TempReg1,(BYTE)SWL_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		Compile_SW_Register(Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}		
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = Map_TempReg(Section,x86_Any,-1,FALSE);
	if (UseTlb) {	
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWL_MASK,"SWL_MASK",Value,Offset,4);
	if (!IsConst(Opcode.rt) || MipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWL_SHIFT,"SWL_SHIFT",shift,Offset,4);
		if (IsConst(Opcode.rt)) {
			MoveConstToX86reg(MipsRegLo(Opcode.rt),Offset);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(MipsRegLo(Opcode.rt),Offset);
		} else {
			MoveVariableToX86reg(&GPR[Opcode.rt].UW[0],GPR_NameLo[Opcode.rt],Offset);
		}
		ShiftRightUnsign(Offset);
		AddX86RegToX86Reg(Value,Offset);
	}

	if (UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		CompileWriteTLBMiss(Section, TempReg1, TempReg2);
		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
}

void Compile_R4300i_SW (BLOCK_SECTION * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (Opcode.base == 29 && SPHack) {
		if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
		TempReg1 = Map_MemoryStack(Section,TRUE);

		if (IsConst(Opcode.rt)) {
			MoveConstToMemoryDisp (MipsRegLo(Opcode.rt),TempReg1, (DWORD)((short)Opcode.offset));
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regToMemory(MipsRegLo(Opcode.rt),TempReg1,(DWORD)((short)Opcode.offset));
		} else {	
			TempReg2 = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			MoveX86regToMemory(TempReg2,TempReg1,(DWORD)((short)Opcode.offset));
		}		
	} else {
		if (IsConst(Opcode.base)) { 
			DWORD Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
			
			if (IsConst(Opcode.rt)) {
				Compile_SW_Const(MipsRegLo(Opcode.rt), Address);
			} else if (IsMapped(Opcode.rt)) {
				Compile_SW_Register(MipsRegLo(Opcode.rt), Address);
			} else {
				Compile_SW_Register(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), Address);
			}
			return;
		}
		if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Section,Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			}
			UnProtectGPR(Section,Opcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
			}
		}
		if (UseTlb) {
			TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
			MoveX86RegToX86Reg(TempReg1, TempReg2);
			ShiftRightUnsignImmed(TempReg2,12);
			MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
			CompileWriteTLBMiss(Section, TempReg1, TempReg2);
			//For tlb miss
			//0041C522 85 C0                test        eax,eax
			//0041C524 75 01                jne         0041C527

			if (IsConst(Opcode.rt)) {
				MoveConstToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else if (IsMapped(Opcode.rt)) {
				MoveX86regToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
			} else {	
				MoveX86regToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
			}
		} else {
			AndConstToX86Reg(TempReg1,0x1FFFFFFF);
			if (IsConst(Opcode.rt)) {
				MoveConstToN64Mem(MipsRegLo(Opcode.rt),TempReg1);
			} else if (IsMapped(Opcode.rt)) {
				MoveX86regToN64Mem(MipsRegLo(Opcode.rt),TempReg1);
			} else {	
				MoveX86regToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1);
			}
		}
	}
}

void Compile_R4300i_SWR (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2, Value, Offset, shift;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (IsConst(Opcode.base)) { 
		DWORD Address;
	
		Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Offset  = Address & 3;
		
		Value = Map_TempReg(Section,x86_Any,-1,FALSE);
		Compile_LW(Value,(Address & ~3));
		AndConstToX86Reg(Value,SWR_MASK[Offset]);
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
		ShiftLeftSignImmed(TempReg1,(BYTE)SWR_SHIFT[Offset]);		
		AddX86RegToX86Reg(Value,TempReg1);		
		Compile_SW_Register(Value, (Address & ~3));
		return;
	}
	shift = Map_TempReg(Section,x86_ECX,-1,FALSE);
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}		
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
	}
	
	Offset = Map_TempReg(Section,x86_Any,-1,FALSE);
	MoveX86RegToX86Reg(TempReg1, Offset);
	AndConstToX86Reg(Offset,3);
	AndConstToX86Reg(TempReg1,~3);

	Value = Map_TempReg(Section,x86_Any,-1,FALSE);
	if (UseTlb) {
		MoveX86regPointerToX86reg(TempReg1, TempReg2,Value);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		MoveN64MemToX86reg(Value,TempReg1);
	}

	AndVariableDispToX86Reg(SWR_MASK,"SWR_MASK",Value,Offset,4);
	if (!IsConst(Opcode.rt) || MipsRegLo(Opcode.rt) != 0) {
		MoveVariableDispToX86Reg(SWR_SHIFT,"SWR_SHIFT",shift,Offset,4);
		if (IsConst(Opcode.rt)) {
			MoveConstToX86reg(MipsRegLo(Opcode.rt),Offset);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86RegToX86Reg(MipsRegLo(Opcode.rt),Offset);
		} else {
			MoveVariableToX86reg(&GPR[Opcode.rt].UW[0],GPR_NameLo[Opcode.rt],Offset);
		}
		ShiftLeftSign(Offset);
		AddX86RegToX86Reg(Value,Offset);
	}

	if (UseTlb) {
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		CompileWriteTLBMiss(Section, TempReg1, TempReg2);
		MoveX86regToX86regPointer(Value,TempReg1, TempReg2);
	} else {
		MoveX86regToN64Mem(Value,TempReg1);
	}
}

void Compile_R4300i_SDL (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_SDL, "r4300i_SDL");
	Popad();

}

void Compile_R4300i_SDR (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.base != 0) { UnMap_GPR(Section,Opcode.base,TRUE); }
	if (Opcode.rt != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_SDR, "r4300i_SDR");
	Popad();

}

void _fastcall ClearRecomplierCache (DWORD Address) {
	if (!TranslateVaddr(&Address)) { DisplayError("Cache: Failed to translate: %X",Address); return; }
	if (Address < RdramSize) {
		DWORD Block = Address >> 12;
		if (N64_Blocks.NoOfRDRamBlocks[Block] > 0) {
			N64_Blocks.NoOfRDRamBlocks[Block] = 0;		
			memset(JumpTable + (Block << 10),0,0x1000);
			*(DelaySlotTable + Block) = NULL;
		}		
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("ClearRecomplierCache: %X",Address);
#endif
	}
}

void Compile_R4300i_CACHE (BLOCK_SECTION * Section){
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (SelfModCheck != ModCode_Cache && 
		SelfModCheck != ModCode_ChangeMemory && 
		SelfModCheck != ModCode_CheckMemory2 && 
		SelfModCheck != ModCode_CheckMemoryCache) 
	{ 
		return; 
	}

	switch(Opcode.rt) {
	case 0:
	case 16:
		Pushad();
		if (IsConst(Opcode.base)) { 
			DWORD Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
			MoveConstToX86reg(Address,x86_ECX);
		} else if (IsMapped(Opcode.base)) { 
			if (MipsRegLo(Opcode.base) == x86_ECX) {
				AddConstToX86Reg(x86_ECX,(short)Opcode.offset);
			} else {
				LeaSourceAndOffset(x86_ECX,MipsRegLo(Opcode.base),(short)Opcode.offset);
			}
		} else {
			MoveVariableToX86reg(&GPR[Opcode.base].UW[0],GPR_NameLo[Opcode.base],x86_ECX);
			AddConstToX86Reg(x86_ECX,(short)Opcode.offset);
		}
		Call_Direct(ClearRecomplierCache, "ClearRecomplierCache");
		Popad();
		break;
	case 1:
	case 3:
	case 13:
	case 5:
	case 8:
	case 9:
	case 17:
	case 21:
	case 25:
		break;
#ifndef EXTERNAL_RELEASE
	default:
		DisplayError("cache: %d",Opcode.rt);
#endif
	}
}

void Compile_R4300i_LL (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		Compile_LW(MipsRegLo(Opcode.rt),Address);
		MoveConstToVariable(1,&LLBit,"LLBit");
		TranslateVaddr(&Address);
		MoveConstToVariable(Address,&LLAddr,"LLAddr");
		return;
	}
	if (UseTlb) {	
		if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
		if (IsMapped(Opcode.base) && Opcode.offset == 0) { 
			ProtectGPR(Section,Opcode.base);
			TempReg1 = MipsRegLo(Opcode.base);
		} else {
			if (IsMapped(Opcode.base)) { 
				ProtectGPR(Section,Opcode.base);
				if (Opcode.offset != 0) {
					TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
					LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
				} else {
					TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
				}
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
				if (Opcode.immediate == 0) { 
				} else if (Opcode.immediate == 1) {
					IncX86reg(TempReg1);
				} else if (Opcode.immediate == 0xFFFF) {			
					DecX86reg(TempReg1);
				} else {
					AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
				}
			}
		}
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		CompileReadTLBMiss(Section,TempReg1,TempReg2);
		Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,MipsRegLo(Opcode.rt));
		MoveConstToVariable(1,&LLBit,"LLBit");
		MoveX86regToVariable(TempReg1,&LLAddr,"LLAddr");
		AddX86regToVariable(TempReg2,&LLAddr,"LLAddr");
		SubConstFromVariable((DWORD)N64MEM,&LLAddr,"LLAddr");
	} else {
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Section,Opcode.base);
			if (Opcode.offset != 0) {
				Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
				LeaSourceAndOffset(MipsRegLo(Opcode.rt),MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
			}
		} else {
			Map_GPR_32bit(Section,Opcode.rt,TRUE,Opcode.base);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(MipsRegLo(Opcode.rt));
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(MipsRegLo(Opcode.rt));
			} else {
				AddConstToX86Reg(MipsRegLo(Opcode.rt),(short)Opcode.immediate);
			}
		}
		AndConstToX86Reg(MipsRegLo(Opcode.rt),0x1FFFFFFF);
		MoveX86regToVariable(MipsRegLo(Opcode.rt),&LLAddr,"LLAddr");
		MoveN64MemToX86reg(MipsRegLo(Opcode.rt),MipsRegLo(Opcode.rt));
		MoveConstToVariable(1,&LLBit,"LLBit");
	}
}

void Compile_R4300i_SC (BLOCK_SECTION * Section){
	DWORD TempReg1, TempReg2;
	BYTE * Jump;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	CompConstToVariable(1,&LLBit,"LLBit");
	JneLabel32("LLBitNotSet",0);
	Jump = RecompPos - 4;
	if (IsConst(Opcode.base)) { 
		DWORD Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
			
		if (IsConst(Opcode.rt)) {
			Compile_SW_Const(MipsRegLo(Opcode.rt), Address);
		} else if (IsMapped(Opcode.rt)) {
			Compile_SW_Register(MipsRegLo(Opcode.rt), Address);
		} else {
			Compile_SW_Register(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), Address);
		}
		CPU_Message("      LLBitNotSet:");
		*((DWORD *)(Jump))=(BYTE)(RecompPos - Jump - 4);
		Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
		MoveVariableToX86reg(&LLBit,"LLBit",MipsRegLo(Opcode.rt));
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
		UnProtectGPR(Section,Opcode.base);
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		CompileWriteTLBMiss(Section, TempReg1, TempReg2);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		if (IsConst(Opcode.rt)) {
			MoveConstToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			MoveX86regToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		if (IsConst(Opcode.rt)) {
			MoveConstToN64Mem(MipsRegLo(Opcode.rt),TempReg1);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regToN64Mem(MipsRegLo(Opcode.rt),TempReg1);
		} else {	
			MoveX86regToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE),TempReg1);
		}
	}
	CPU_Message("      LLBitNotSet:");
	*((DWORD *)(Jump))=(BYTE)(RecompPos - Jump - 4);
	Map_GPR_32bit(Section,Opcode.rt,FALSE,-1);
	MoveVariableToX86reg(&LLBit,"LLBit",MipsRegLo(Opcode.rt));

}

void Compile_R4300i_LD (BLOCK_SECTION * Section) {
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rt == 0) return;

	if (IsConst(Opcode.base)) { 
		DWORD Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
		Map_GPR_64bit(Section,Opcode.rt,-1);
		Compile_LW(MipsRegHi(Opcode.rt),Address);
		Compile_LW(MipsRegLo(Opcode.rt),Address + 4);
		if (SPHack && Opcode.rt == 29) { ResetMemoryStack(Section); }
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base) && Opcode.offset == 0) { 
		if (UseTlb) {
			ProtectGPR(Section,Opcode.base);
			TempReg1 = MipsRegLo(Opcode.base);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		if (IsMapped(Opcode.base)) { 
			ProtectGPR(Section,Opcode.base);
			if (Opcode.offset != 0) {
				TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
				LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
			} else {
				TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			}
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
			if (Opcode.immediate == 0) { 
			} else if (Opcode.immediate == 1) {
				IncX86reg(TempReg1);
			} else if (Opcode.immediate == 0xFFFF) {			
				DecX86reg(TempReg1);
			} else {
				AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
			}
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_ReadMap,"TLB_ReadMap",TempReg2,TempReg2,4);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527
		Map_GPR_64bit(Section,Opcode.rt,-1);
		MoveX86regPointerToX86reg(TempReg1, TempReg2,MipsRegHi(Opcode.rt));
		MoveX86regPointerToX86regDisp8(TempReg1, TempReg2,MipsRegLo(Opcode.rt),4);
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);
		Map_GPR_64bit(Section,Opcode.rt,-1);
		MoveN64MemToX86reg(MipsRegHi(Opcode.rt),TempReg1);
		MoveN64MemDispToX86reg(MipsRegLo(Opcode.rt),TempReg1,4);
	}
	if (SPHack && Opcode.rt == 29) { 
		int count;

		for (count = 0; count < 10; count ++) { x86Protected(count) = FALSE; }
		ResetMemoryStack(Section); 
	}
}

void Compile_R4300i_SD (BLOCK_SECTION * Section){
	DWORD TempReg1, TempReg2;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (IsConst(Opcode.base)) { 
		DWORD Address = MipsRegLo(Opcode.base) + (short)Opcode.offset;
		
		if (IsConst(Opcode.rt)) {
			if (Is64Bit(Opcode.rt)) {
				Compile_SW_Const(MipsRegHi(Opcode.rt), Address);
			} else {
				Compile_SW_Const((MipsRegLo_S(Opcode.rt) >> 31), Address);
			}
			Compile_SW_Const(MipsRegLo(Opcode.rt), Address + 4);
		} else if (IsMapped(Opcode.rt)) {
			if (Is64Bit(Opcode.rt)) {
				Compile_SW_Register(MipsRegHi(Opcode.rt), Address);
			} else {
				Compile_SW_Register(Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), Address);
			}
			Compile_SW_Register(MipsRegLo(Opcode.rt), Address + 4);		
		} else {
			Compile_SW_Register(TempReg1 = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), Address);
			Compile_SW_Register(Map_TempReg(Section,TempReg1,Opcode.rt,FALSE), Address + 4);		
		}
		return;
	}
	if (IsMapped(Opcode.rt)) { ProtectGPR(Section,Opcode.rt); }
	if (IsMapped(Opcode.base)) { 
		ProtectGPR(Section,Opcode.base);
		if (Opcode.offset != 0) {
			TempReg1 = Map_TempReg(Section,x86_Any,-1,FALSE);
			LeaSourceAndOffset(TempReg1,MipsRegLo(Opcode.base),(short)Opcode.offset);
		} else {
			TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		}
	} else {
		TempReg1 = Map_TempReg(Section,x86_Any,Opcode.base,FALSE);
		if (Opcode.immediate == 0) { 
		} else if (Opcode.immediate == 1) {
			IncX86reg(TempReg1);
		} else if (Opcode.immediate == 0xFFFF) {			
			DecX86reg(TempReg1);
		} else {
			AddConstToX86Reg(TempReg1,(short)Opcode.immediate);
		}
	}
	if (UseTlb) {
		TempReg2 = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveX86RegToX86Reg(TempReg1, TempReg2);
		ShiftRightUnsignImmed(TempReg2,12);
		MoveVariableDispToX86Reg(TLB_WriteMap,"TLB_WriteMap",TempReg2,TempReg2,4);
		CompileWriteTLBMiss(Section, TempReg1, TempReg2);
		//For tlb miss
		//0041C522 85 C0                test        eax,eax
		//0041C524 75 01                jne         0041C527

		if (IsConst(Opcode.rt)) {
			if (Is64Bit(Opcode.rt)) {
				MoveConstToX86regPointer(MipsRegHi(Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveConstToX86regPointer((MipsRegLo_S(Opcode.rt) >> 31),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveConstToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else if (IsMapped(Opcode.rt)) {
			if (Is64Bit(Opcode.rt)) {
				MoveX86regToX86regPointer(MipsRegHi(Opcode.rt),TempReg1, TempReg2);
			} else {
				MoveX86regToX86regPointer(Map_TempReg(Section,x86_Any,Opcode.rt,TRUE),TempReg1, TempReg2);
			}
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(MipsRegLo(Opcode.rt),TempReg1, TempReg2);
		} else {	
			int X86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			MoveX86regToX86regPointer(X86Reg,TempReg1, TempReg2);
			AddConstToX86Reg(TempReg1,4);
			MoveX86regToX86regPointer(Map_TempReg(Section,X86Reg,Opcode.rt,FALSE),TempReg1, TempReg2);
		}
	} else {
		AndConstToX86Reg(TempReg1,0x1FFFFFFF);		
		if (IsConst(Opcode.rt)) {
			if (Is64Bit(Opcode.rt)) {
				MoveConstToN64Mem(MipsRegHi(Opcode.rt),TempReg1);
			} else if (IsSigned(Opcode.rt)) {
				MoveConstToN64Mem(((int)MipsRegLo(Opcode.rt) >> 31),TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveConstToN64MemDisp(MipsRegLo(Opcode.rt),TempReg1,4);
		} else if (IsKnown(Opcode.rt) && IsMapped(Opcode.rt)) {
			if (Is64Bit(Opcode.rt)) {
				MoveX86regToN64Mem(MipsRegHi(Opcode.rt),TempReg1);
			} else if (IsSigned(Opcode.rt)) {
				MoveX86regToN64Mem(Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), TempReg1);
			} else {
				MoveConstToN64Mem(0,TempReg1);
			}
			MoveX86regToN64MemDisp(MipsRegLo(Opcode.rt),TempReg1, 4);		
		} else {	
			int x86reg;
			MoveX86regToN64Mem(x86reg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE), TempReg1);
			MoveX86regToN64MemDisp(Map_TempReg(Section,x86reg,Opcode.rt,FALSE), TempReg1,4);
		}
	}
}

/********************** R4300i OpCodes: Special **********************/
void Compile_R4300i_SPECIAL_SLL (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (IsConst(Opcode.rt)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegLo(Opcode.rd) = MipsRegLo(Opcode.rt) << Opcode.sa;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		return;
	}
	if (Opcode.rd != Opcode.rt && IsMapped(Opcode.rt)) {
		switch (Opcode.sa) {
		case 0: 
			Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
			break;
		case 1:
			ProtectGPR(Section,Opcode.rt);
			Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			LeaRegReg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt), 2);
			break;			
		case 2:
			ProtectGPR(Section,Opcode.rt);
			Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			LeaRegReg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt), 4);
			break;			
		case 3:
			ProtectGPR(Section,Opcode.rt);
			Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			LeaRegReg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt), 8);
			break;
		default:
			Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
			ShiftLeftSignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	} else {
		Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftLeftSignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
	}
}

void Compile_R4300i_SPECIAL_SRL (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsConst(Opcode.rt)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegLo(Opcode.rd) = MipsRegLo(Opcode.rt) >> Opcode.sa;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightUnsignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_SRA (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsConst(Opcode.rt)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegLo(Opcode.rd) = MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		return;
	}
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightSignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_SLLV (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsConst(Opcode.rs)) {
		DWORD Shift = (MipsRegLo(Opcode.rs) & 0x1F);
		if (IsConst(Opcode.rt)) {
			if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			MipsRegLo(Opcode.rd) = MipsRegLo(Opcode.rt) << Shift;
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
			ShiftLeftSignImmed(MipsRegLo(Opcode.rd),(BYTE)Shift);
		}
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftLeftSign(MipsRegLo(Opcode.rd));
}

void Compile_R4300i_SPECIAL_SRLV (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsKnown(Opcode.rs) && IsConst(Opcode.rs)) {
		DWORD Shift = (MipsRegLo(Opcode.rs) & 0x1F);
		if (IsConst(Opcode.rt)) {
			if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			MipsRegLo(Opcode.rd) = MipsRegLo(Opcode.rt) >> Shift;
			MipsRegState(Opcode.rd) = STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftRightUnsignImmed(MipsRegLo(Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightUnsign(MipsRegLo(Opcode.rd));
}

void Compile_R4300i_SPECIAL_SRAV (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsKnown(Opcode.rs) && IsConst(Opcode.rs)) {
		DWORD Shift = (MipsRegLo(Opcode.rs) & 0x1F);
		if (IsConst(Opcode.rt)) {
			if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			MipsRegLo(Opcode.rd) = MipsRegLo_S(Opcode.rt) >> Shift;
			MipsRegState(Opcode.rd) = STATE_CONST_32;
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
		ShiftRightSignImmed(MipsRegLo(Opcode.rd),(BYTE)Shift);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x1F);
	Map_GPR_32bit(Section,Opcode.rd,TRUE,Opcode.rt);
	ShiftRightSign(MipsRegLo(Opcode.rd));
}

void Compile_R4300i_SPECIAL_JR (BLOCK_SECTION * Section) {
	static char JumpLabel[100];

	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		if (IsConst(Opcode.rs)) { 
			sprintf(JumpLabel,"0x%08X",MipsRegLo(Opcode.rs));
			Section->Jump.BranchLabel   = JumpLabel;
			Section->Jump.TargetPC      = MipsRegLo(Opcode.rs);
			Section->Jump.FallThrough   = TRUE;
			Section->Jump.LinkLocation  = NULL;
			Section->Jump.LinkLocation2 = NULL;
			Section->Cont.FallThrough   = FALSE;
			Section->Cont.LinkLocation  = NULL;
			Section->Cont.LinkLocation2 = NULL;
			if ((Section->CompilePC & 0xFFC) == 0xFFC) {
				GenerateSectionLinkage(Section);
				NextInstruction = END_BLOCK;
				return;
			}
		}
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			if (IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(MipsRegLo(Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(Section->CompilePC + 4,&PROGRAM_COUNTER,"PROGRAM_COUNTER");
			if (BlockCycleCount != 0) { 
				AddConstToVariable(BlockCycleCount,&CP0[9],Cop0_Name[9]); 
				SubConstFromVariable(BlockCycleCount,&Timers.Timer,"Timer");
			}
			if (BlockRandomModifier != 0) { SubConstFromVariable(BlockRandomModifier,&CP0[1],Cop0_Name[1]); }
			WriteBackRegisters(Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		if (DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0)) {
			if (IsConst(Opcode.rs)) { 
				MoveConstToVariable(MipsRegLo(Opcode.rs),&PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else 	if (IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(MipsRegLo(Opcode.rs),&PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),&PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0)) {
			CompileExit((DWORD)-1,Section->RegWorking,Normal,TRUE,NULL);
		} else {
			if (IsConst(Opcode.rs)) { 
				memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
				GenerateSectionLinkage(Section);
			} else {
				if (IsMapped(Opcode.rs)) { 
					MoveX86regToVariable(MipsRegLo(Opcode.rs),&PROGRAM_COUNTER, "PROGRAM_COUNTER");
				} else {
					MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),&PROGRAM_COUNTER, "PROGRAM_COUNTER");
				}
				CompileExit((DWORD)-1,Section->RegWorking,Normal,TRUE,NULL);
			}
		}
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_SPECIAL_JALR (BLOCK_SECTION * Section) {
	static char JumpLabel[100];	
	
	if ( NextInstruction == NORMAL ) {
		CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
		if (DelaySlotEffectsCompare(Section->CompilePC,Opcode.rs,0)) {
			Compile_R4300i_UnknownOpcode(Section);
		}
		UnMap_GPR(Section, Opcode.rd, FALSE);
		MipsRegLo(Opcode.rd) = Section->CompilePC + 8;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		if ((Section->CompilePC & 0xFFC) == 0xFFC) {
			if (IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(MipsRegLo(Opcode.rs),&JumpToLocation, "JumpToLocation");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),&JumpToLocation, "JumpToLocation");
			}
			MoveConstToVariable(Section->CompilePC + 4,&PROGRAM_COUNTER,"PROGRAM_COUNTER");
			if (BlockCycleCount != 0) { 
				AddConstToVariable(BlockCycleCount,&CP0[9],Cop0_Name[9]); 
				SubConstFromVariable(BlockCycleCount,&Timers.Timer,"Timer");
			}
			if (BlockRandomModifier != 0) { SubConstFromVariable(BlockRandomModifier,&CP0[1],Cop0_Name[1]); }
			WriteBackRegisters(Section);
			if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
			MoveConstToVariable(DELAY_SLOT,&NextInstruction,"NextInstruction");
			Ret();
			NextInstruction = END_BLOCK;
			return;
		}
		NextInstruction = DO_DELAY_SLOT;
	} else if (NextInstruction == DELAY_SLOT_DONE ) {		
		if (IsConst(Opcode.rs)) { 
			memcpy(&Section->Jump.RegSet,&Section->RegWorking,sizeof(REG_INFO));
			sprintf(JumpLabel,"0x%08X",MipsRegLo(Opcode.rs));
			Section->Jump.BranchLabel   = JumpLabel;
			Section->Jump.TargetPC      = MipsRegLo(Opcode.rs);
			Section->Jump.FallThrough   = TRUE;
			Section->Jump.LinkLocation  = NULL;
			Section->Jump.LinkLocation2 = NULL;
			Section->Cont.FallThrough   = FALSE;
			Section->Cont.LinkLocation  = NULL;
			Section->Cont.LinkLocation2 = NULL;

			GenerateSectionLinkage(Section);
		} else {
			if (IsMapped(Opcode.rs)) { 
				MoveX86regToVariable(MipsRegLo(Opcode.rs),&PROGRAM_COUNTER, "PROGRAM_COUNTER");
			} else {
				MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,FALSE),&PROGRAM_COUNTER, "PROGRAM_COUNTER");
			}
			CompileExit((DWORD)-1,Section->RegWorking,Normal,TRUE,NULL);
		}
		NextInstruction = END_BLOCK;
	} else {
#ifndef EXTERNAL_RELEASE
		DisplayError("WTF\n\nBranch\nNextInstruction = %X", NextInstruction);
#endif
	}
}

void Compile_R4300i_SPECIAL_SYSCALL (BLOCK_SECTION * Section) {
	CompileExit(Section->CompilePC,Section->RegWorking,DoSysCall,TRUE,NULL);
}

void Compile_R4300i_SPECIAL_MFLO (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	Map_GPR_64bit(Section,Opcode.rd,-1);
	MoveVariableToX86reg(&LO.UW[0],"LO.UW[0]",MipsRegLo(Opcode.rd));
	MoveVariableToX86reg(&LO.UW[1],"LO.UW[1]",MipsRegHi(Opcode.rd));
}

void Compile_R4300i_SPECIAL_MTLO (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (IsKnown(Opcode.rs) && IsConst(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			MoveConstToVariable(MipsRegHi(Opcode.rs),&LO.UW[1],"LO.UW[1]");
		} else if (IsSigned(Opcode.rs) && ((MipsRegLo(Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&LO.UW[1],"LO.UW[1]");
		} else {
			MoveConstToVariable(0,&LO.UW[1],"LO.UW[1]");
		}
		MoveConstToVariable(MipsRegLo(Opcode.rs), &LO.UW[0],"LO.UW[0]");
	} else if (IsKnown(Opcode.rs) && IsMapped(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			MoveX86regToVariable(MipsRegHi(Opcode.rs),&LO.UW[1],"LO.UW[1]");
		} else if (IsSigned(Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,TRUE),&LO.UW[1],"LO.UW[1]");
		} else {
			MoveConstToVariable(0,&LO.UW[1],"LO.UW[1]");
		}
		MoveX86regToVariable(MipsRegLo(Opcode.rs), &LO.UW[0],"LO.UW[0]");
	} else {
		int x86reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&LO.UW[1],"LO.UW[1]");
		MoveX86regToVariable(Map_TempReg(Section,x86reg,Opcode.rs,FALSE), &LO.UW[0],"LO.UW[0]");
	}
}

void Compile_R4300i_SPECIAL_MFHI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	Map_GPR_64bit(Section,Opcode.rd,-1);
	MoveVariableToX86reg(&HI.UW[0],"HI.UW[0]",MipsRegLo(Opcode.rd));
	MoveVariableToX86reg(&HI.UW[1],"HI.UW[1]",MipsRegHi(Opcode.rd));
}

void Compile_R4300i_SPECIAL_MTHI (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (IsKnown(Opcode.rs) && IsConst(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			MoveConstToVariable(MipsRegHi(Opcode.rs),&HI.UW[1],"HI.UW[1]");
		} else if (IsSigned(Opcode.rs) && ((MipsRegLo(Opcode.rs) & 0x80000000) != 0)) {
			MoveConstToVariable(0xFFFFFFFF,&HI.UW[1],"HI.UW[1]");
		} else {
			MoveConstToVariable(0,&HI.UW[1],"HI.UW[1]");
		}
		MoveConstToVariable(MipsRegLo(Opcode.rs), &HI.UW[0],"HI.UW[0]");
	} else if (IsKnown(Opcode.rs) && IsMapped(Opcode.rs)) {
		if (Is64Bit(Opcode.rs)) {
			MoveX86regToVariable(MipsRegHi(Opcode.rs),&HI.UW[1],"HI.UW[1]");
		} else if (IsSigned(Opcode.rs)) {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rs,TRUE),&HI.UW[1],"HI.UW[1]");
		} else {
			MoveConstToVariable(0,&HI.UW[1],"HI.UW[1]");
		}
		MoveX86regToVariable(MipsRegLo(Opcode.rs), &HI.UW[0],"HI.UW[0]");
	} else {
		int x86reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		MoveX86regToVariable(x86reg,&HI.UW[1],"HI.UW[1]");
		MoveX86regToVariable(Map_TempReg(Section,x86reg,Opcode.rs,FALSE), &HI.UW[0],"HI.UW[0]");
	}
}

void Compile_R4300i_SPECIAL_DSLLV (BLOCK_SECTION * Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsConst(Opcode.rs)) {
		DWORD Shift = (MipsRegLo(Opcode.rs) & 0x3F);
		Compile_R4300i_UnknownOpcode(Section);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftLeftDouble(MipsRegHi(Opcode.rd),MipsRegLo(Opcode.rd));
	ShiftLeftSign(MipsRegLo(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegHi(Opcode.rd));
	XorX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftLeftSign(MipsRegHi(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void Compile_R4300i_SPECIAL_DSRLV (BLOCK_SECTION * Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsConst(Opcode.rs)) {
		DWORD Shift = (MipsRegLo(Opcode.rs) & 0x3F);
		if (IsConst(Opcode.rt)) {
			if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			MipsReg(Opcode.rd) = Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt);
			MipsReg(Opcode.rd) = MipsReg(Opcode.rd) >> Shift;
			if ((MipsRegHi(Opcode.rd) == 0) && (MipsRegLo(Opcode.rd) & 0x80000000) == 0) {
				MipsRegState(Opcode.rd) = STATE_CONST_32;
			} else if ((MipsRegHi(Opcode.rd) == 0xFFFFFFFF) && (MipsRegLo(Opcode.rd) & 0x80000000) != 0) {
				MipsRegState(Opcode.rd) = STATE_CONST_32;
			} else {
				MipsRegState(Opcode.rd) = STATE_CONST_64;
			}
			return;
		}
		//if (Shift < 0x20) {
		//} else {
		//}
		//Compile_R4300i_UnknownOpcode(Section);
		//return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftRightDouble(MipsRegLo(Opcode.rd),MipsRegHi(Opcode.rd));
	ShiftRightUnsign(MipsRegHi(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegLo(Opcode.rd));
	XorX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(Opcode.rd));
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightUnsign(MipsRegLo(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void Compile_R4300i_SPECIAL_DSRAV (BLOCK_SECTION * Section) {
	BYTE * Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }
	
	if (IsConst(Opcode.rs)) {
		DWORD Shift = (MipsRegLo(Opcode.rs) & 0x3F);
		Compile_R4300i_UnknownOpcode(Section);
		return;
	}
	Map_TempReg(Section,x86_ECX,Opcode.rs,FALSE);
	AndConstToX86Reg(x86_ECX,0x3F);
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	CompConstToX86reg(x86_ECX,0x20);
	JaeLabel8("MORE32", 0);
	Jump[0] = RecompPos - 1;
	ShiftRightDouble(MipsRegLo(Opcode.rd),MipsRegHi(Opcode.rd));
	ShiftRightSign(MipsRegHi(Opcode.rd));
	JmpLabel8("continue", 0);
	Jump[1] = RecompPos - 1;
	
	//MORE32:
	CPU_Message("");
	CPU_Message("      MORE32:");
	*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	MoveX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegLo(Opcode.rd));
	ShiftRightSignImmed(MipsRegHi(Opcode.rd),0x1F);
	AndConstToX86Reg(x86_ECX,0x1F);
	ShiftRightSign(MipsRegLo(Opcode.rd));

	//continue:
	CPU_Message("");
	CPU_Message("      continue:");
	*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
}

void Compile_R4300i_SPECIAL_MULT ( BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	imulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&LO.UW[0],"LO.UW[0]");
	MoveX86regToVariable(x86_EDX,&HI.UW[0],"HI.UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&LO.UW[1],"LO.UW[1]");
	MoveX86regToVariable(x86_EDX,&HI.UW[1],"HI.UW[1]");
}

void Compile_R4300i_SPECIAL_MULTU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	MulX86reg(x86_EDX);

	MoveX86regToVariable(x86_EAX,&LO.UW[0],"LO.UW[0]");
	MoveX86regToVariable(x86_EDX,&HI.UW[0],"HI.UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&LO.UW[1],"LO.UW[1]");
	MoveX86regToVariable(x86_EDX,&HI.UW[1],"HI.UW[1]");
}

void Compile_R4300i_SPECIAL_DIV (BLOCK_SECTION * Section) {
	BYTE *Jump[2];

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (IsConst(Opcode.rt)) {
		if (MipsRegLo(Opcode.rt) == 0) {
			MoveConstToVariable(0, &LO.UW[0], "LO.UW[0]");
			MoveConstToVariable(0, &LO.UW[1], "LO.UW[1]");
			MoveConstToVariable(0, &HI.UW[0], "HI.UW[0]");
			MoveConstToVariable(0, &HI.UW[1], "HI.UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (IsMapped(Opcode.rt)) {
			CompConstToX86reg(MipsRegLo(Opcode.rt),0);
		} else {
			CompConstToVariable(0, &GPR[Opcode.rt].W[0], GPR_NameLo[Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = RecompPos - 1;

		MoveConstToVariable(0, &LO.UW[0], "LO.UW[0]");
		MoveConstToVariable(0, &LO.UW[1], "LO.UW[1]");
		MoveConstToVariable(0, &HI.UW[0], "HI.UW[0]");
		MoveConstToVariable(0, &HI.UW[1], "HI.UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	}
	/*	lo = (SD)rs / (SD)rt;
		hi = (SD)rs % (SD)rt; */

	x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);

	/* edx is the signed portion to eax */
	x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX, -1, FALSE);

	MoveX86RegToX86Reg(x86_EAX, x86_EDX);
	ShiftRightSignImmed(x86_EDX,31);

	if (IsMapped(Opcode.rt)) {
		idivX86reg(MipsRegLo(Opcode.rt));
	} else {
		idivX86reg(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE));
	}
		

	MoveX86regToVariable(x86_EAX,&LO.UW[0],"LO.UW[0]");
	MoveX86regToVariable(x86_EDX,&HI.UW[0],"HI.UW[0]");
	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&LO.UW[1],"LO.UW[1]");
	MoveX86regToVariable(x86_EDX,&HI.UW[1],"HI.UW[1]");

	if( Jump[1] != NULL ) {
		CPU_Message("");
		CPU_Message("      EndDivu:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
	}
}

void Compile_R4300i_SPECIAL_DIVU ( BLOCK_SECTION * Section) {
	BYTE *Jump[2];
	int x86reg;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (IsConst(Opcode.rt)) {
		if (MipsRegLo(Opcode.rt) == 0) {
			MoveConstToVariable(0, &LO.UW[0], "LO.UW[0]");
			MoveConstToVariable(0, &LO.UW[1], "LO.UW[1]");
			MoveConstToVariable(0, &HI.UW[0], "HI.UW[0]");
			MoveConstToVariable(0, &HI.UW[1], "HI.UW[1]");
			return;
		}
		Jump[1] = NULL;
	} else {
		if (IsMapped(Opcode.rt)) {
			CompConstToX86reg(MipsRegLo(Opcode.rt),0);
		} else {
			CompConstToVariable(0, &GPR[Opcode.rt].W[0], GPR_NameLo[Opcode.rt]);
		}
		JneLabel8("NoExcept", 0);
		Jump[0] = RecompPos - 1;

		MoveConstToVariable(0, &LO.UW[0], "LO.UW[0]");
		MoveConstToVariable(0, &LO.UW[1], "LO.UW[1]");
		MoveConstToVariable(0, &HI.UW[0], "HI.UW[0]");
		MoveConstToVariable(0, &HI.UW[1], "HI.UW[1]");

		JmpLabel8("EndDivu", 0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      NoExcept:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
	}


	/*	lo = (UD)rs / (UD)rt;
		hi = (UD)rs % (UD)rt; */

	x86Protected(x86_EAX) = TRUE;
	Map_TempReg(Section,x86_EDX, 0, FALSE);
	x86Protected(x86_EAX) = FALSE;

	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	x86reg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);

	DivX86reg(x86reg);

	MoveX86regToVariable(x86_EAX,&LO.UW[0],"LO.UW[0]");
	MoveX86regToVariable(x86_EDX,&HI.UW[0],"HI.UW[0]");

	/* wouldnt these be zero (???) */

	ShiftRightSignImmed(x86_EAX,31);	/* paired */
	ShiftRightSignImmed(x86_EDX,31);
	MoveX86regToVariable(x86_EAX,&LO.UW[1],"LO.UW[1]");
	MoveX86regToVariable(x86_EDX,&HI.UW[1],"HI.UW[1]");

	if( Jump[1] != NULL ) {
		CPU_Message("");
		CPU_Message("      EndDivu:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
	}
}

void Compile_R4300i_SPECIAL_DMULT (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rs,TRUE); }
	if (Opcode.rs != 0) { UnMap_GPR(Section,Opcode.rt,TRUE); }
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_SPECIAL_DMULT, "r4300i_SPECIAL_DMULT");
	Popad();
}

void Compile_R4300i_SPECIAL_DMULTU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	/* LO.UDW = (uint64)GPR[Opcode.rs].UW[0] * (uint64)GPR[Opcode.rt].UW[0]; */
	x86Protected(x86_EDX) = TRUE;
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	x86Protected(x86_EDX) = FALSE;
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &LO.UW[0], "LO.UW[0]");
	MoveX86regToVariable(x86_EDX, &LO.UW[1], "LO.UW[1]");

	/* HI.UDW = (uint64)GPR[Opcode.rs].UW[1] * (uint64)GPR[Opcode.rt].UW[1]; */
	Map_TempReg(Section,x86_EAX,Opcode.rs,TRUE);
	Map_TempReg(Section,x86_EDX,Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	MoveX86regToVariable(x86_EAX, &HI.UW[0], "HI.UW[0]");
	MoveX86regToVariable(x86_EDX, &HI.UW[1], "HI.UW[1]");

	/* Tmp[0].UDW = (uint64)GPR[Opcode.rs].UW[1] * (uint64)GPR[Opcode.rt].UW[0]; */
	Map_TempReg(Section,x86_EAX,Opcode.rs,TRUE);
	Map_TempReg(Section,x86_EDX,Opcode.rt,FALSE);

	Map_TempReg(Section,x86_EBX,-1,FALSE);
	Map_TempReg(Section,x86_ECX,-1,FALSE);

	MulX86reg(x86_EDX);
	MoveX86RegToX86Reg(x86_EAX, x86_EBX); /* EDX:EAX -> ECX:EBX */
	MoveX86RegToX86Reg(x86_EDX, x86_ECX);

	/* Tmp[1].UDW = (uint64)GPR[Opcode.rs].UW[0] * (uint64)GPR[Opcode.rt].UW[1]; */
	Map_TempReg(Section,x86_EAX,Opcode.rs,FALSE);
	Map_TempReg(Section,x86_EDX,Opcode.rt,TRUE);

	MulX86reg(x86_EDX);
	Map_TempReg(Section,x86_ESI,-1,FALSE);
	Map_TempReg(Section,x86_EDI,-1,FALSE);
	MoveX86RegToX86Reg(x86_EAX, x86_ESI); /* EDX:EAX -> EDI:ESI */
	MoveX86RegToX86Reg(x86_EDX, x86_EDI);

	/* Tmp[2].UDW = (uint64)LO.UW[1] + (uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]; */
	XorX86RegToX86Reg(x86_EDX, x86_EDX);
	MoveVariableToX86reg(&LO.UW[1], "LO.UW[1]", x86_EAX);
	AddX86RegToX86Reg(x86_EAX, x86_EBX);
	AddConstToX86Reg(x86_EDX, 0);
	AddX86RegToX86Reg(x86_EAX, x86_ESI);
	AddConstToX86Reg(x86_EDX, 0);			/* EDX:EAX */

	/* LO.UDW += ((uint64)Tmp[0].UW[0] + (uint64)Tmp[1].UW[0]) << 32; */
	/* [low+4] += ebx + esi */

	AddX86regToVariable(x86_EBX, &LO.UW[1], "LO.UW[1]");
	AddX86regToVariable(x86_ESI, &LO.UW[1], "LO.UW[1]");

	/* HI.UDW += (uint64)Tmp[0].UW[1] + (uint64)Tmp[1].UW[1] + Tmp[2].UW[1]; */
	/* [hi] += ecx + edi + edx */
	
	AddX86regToVariable(x86_ECX, &HI.UW[0], "HI.UW[0]");
	AdcConstToVariable(&HI.UW[1], "HI.UW[1]", 0);

	AddX86regToVariable(x86_EDI, &HI.UW[0], "HI.UW[0]");
	AdcConstToVariable(&HI.UW[1], "HI.UW[1]", 0);

	AddX86regToVariable(x86_EDX, &HI.UW[0], "HI.UW[0]");
	AdcConstToVariable(&HI.UW[1], "HI.UW[1]", 0);
}

void Compile_R4300i_SPECIAL_DDIV (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	UnMap_GPR(Section,Opcode.rs,TRUE);
	UnMap_GPR(Section,Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_SPECIAL_DDIV, "r4300i_SPECIAL_DDIV");
	Popad();
}

void Compile_R4300i_SPECIAL_DDIVU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	UnMap_GPR(Section,Opcode.rs,TRUE);
	UnMap_GPR(Section,Opcode.rt,TRUE);
	Pushad();
	MoveConstToVariable(Opcode.Hex, &Opcode.Hex, "Opcode.Hex" );
	Call_Direct(r4300i_SPECIAL_DDIVU, "r4300i_SPECIAL_DDIVU");
	Popad();
}

void Compile_R4300i_SPECIAL_ADD (BLOCK_SECTION * Section) {
	int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
	int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(source1) && IsConst(source2)) {
		DWORD temp = MipsRegLo(source1) + MipsRegLo(source2);
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegLo(Opcode.rd) = temp;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(Section,Opcode.rd,TRUE, source1);
	if (IsConst(source2)) {
		if (MipsRegLo(source2) != 0) {
			AddConstToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
		}
	} else if (IsKnown(source2) && IsMapped(source2)) {
		AddX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
	} else {
		AddVariableToX86reg(MipsRegLo(Opcode.rd),&GPR[source2].W[0],GPR_NameLo[source2]);
	}
}

void Compile_R4300i_SPECIAL_ADDU (BLOCK_SECTION * Section) {
	int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
	int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(source1) && IsConst(source2)) {
		DWORD temp = MipsRegLo(source1) + MipsRegLo(source2);
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegLo(Opcode.rd) = temp;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		return;
	}

	Map_GPR_32bit(Section,Opcode.rd,TRUE, source1);
	if (IsConst(source2)) {
		if (MipsRegLo(source2) != 0) {
			AddConstToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
		}
	} else if (IsKnown(source2) && IsMapped(source2)) {
		AddX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
	} else {
		AddVariableToX86reg(MipsRegLo(Opcode.rd),&GPR[source2].W[0],GPR_NameLo[source2]);
	}
}

void Compile_R4300i_SPECIAL_SUB (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(Opcode.rt)  && IsConst(Opcode.rs)) {
		DWORD temp = MipsRegLo(Opcode.rs) - MipsRegLo(Opcode.rt);
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegLo(Opcode.rd) = temp;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
	} else {
		if (Opcode.rd == Opcode.rt) {
			int x86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
		if (IsConst(Opcode.rt)) {
			SubConstFromX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
		} else if (IsMapped(Opcode.rt)) {
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
		} else {
			SubVariableFromX86reg(MipsRegLo(Opcode.rd),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_SUBU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(Opcode.rt)  && IsConst(Opcode.rs)) {
		DWORD temp = MipsRegLo(Opcode.rs) - MipsRegLo(Opcode.rt);
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegLo(Opcode.rd) = temp;
		MipsRegState(Opcode.rd) = STATE_CONST_32;
	} else {
		if (Opcode.rd == Opcode.rt) {
			int x86Reg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),x86Reg);
			return;
		}
		Map_GPR_32bit(Section,Opcode.rd,TRUE, Opcode.rs);
		if (IsConst(Opcode.rt)) {
			SubConstFromX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
		} else if (IsMapped(Opcode.rt)) {
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
		} else {
			SubVariableFromX86reg(MipsRegLo(Opcode.rd),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_AND (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (IsKnown(Opcode.rt) && IsKnown(Opcode.rs)) {
		if (IsConst(Opcode.rt) && IsConst(Opcode.rs)) {
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				MipsReg(Opcode.rd) = 
					(Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt)) &
					(Is64Bit(Opcode.rs)?MipsReg(Opcode.rs):(_int64)MipsRegLo_S(Opcode.rs));
				
				if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
					MipsRegState(Opcode.rd) = STATE_CONST_32;
				} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
					MipsRegState(Opcode.rd) = STATE_CONST_32;
				} else {
					MipsRegState(Opcode.rd) = STATE_CONST_64;
				}
			} else {
				MipsReg(Opcode.rd) = MipsRegLo(Opcode.rt) & MipsReg(Opcode.rs);
				MipsRegState(Opcode.rd) = STATE_CONST_32;
			}			
		} else if (IsMapped(Opcode.rt) && IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
		
			ProtectGPR(Section,source1);
			ProtectGPR(Section,source2);
			if (Is32Bit(source1) && Is32Bit(source2)) {
				int Sign = (IsSigned(Opcode.rt) && IsSigned(Opcode.rs))?TRUE:FALSE;
				Map_GPR_32bit(Section,Opcode.rd,Sign,source1);				
				AndX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
			} else if (Is32Bit(source1) || Is32Bit(source2)) {
				if (IsUnsigned(Is32Bit(source1)?source1:source2)) {
					Map_GPR_32bit(Section,Opcode.rd,FALSE,source1);
					AndX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
				} else {
					Map_GPR_64bit(Section,Opcode.rd,source1);
					if (Is32Bit(source2)) {
						AndX86RegToX86Reg(MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
					} else {
						AndX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(source2));
					}
					AndX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
				}
			} else {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				AndX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(source2));
				AndX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
			}
		} else {
			int ConstReg = IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			int MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Is64Bit(ConstReg)) {
				if (Is32Bit(MappedReg) && IsUnsigned(MappedReg)) {
					if (MipsRegLo(ConstReg) == 0) {
						Map_GPR_32bit(Section,Opcode.rd,FALSE, 0);
					} else {
						DWORD Value = MipsRegLo(ConstReg);
						Map_GPR_32bit(Section,Opcode.rd,FALSE, MappedReg);
						AndConstToX86Reg(MipsRegLo(Opcode.rd),Value);
					}
				} else {
					_int64 Value = MipsReg(ConstReg);
					Map_GPR_64bit(Section,Opcode.rd,MappedReg);
					AndConstToX86Reg(MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
					AndConstToX86Reg(MipsRegLo(Opcode.rd),(DWORD)Value);
				}
			} else if (Is64Bit(MappedReg)) {
				DWORD Value = MipsRegLo(ConstReg); 
				if (Value != 0) {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(ConstReg)?TRUE:FALSE,MappedReg);					
					AndConstToX86Reg(MipsRegLo(Opcode.rd),(DWORD)Value);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(ConstReg)?TRUE:FALSE, 0);
				}
			} else {
				DWORD Value = MipsRegLo(ConstReg); 
				int Sign = FALSE;
				if (IsSigned(ConstReg) && IsSigned(MappedReg)) { Sign = TRUE; }				
				if (Value != 0) {
					Map_GPR_32bit(Section,Opcode.rd,Sign,MappedReg);
					AndConstToX86Reg(MipsRegLo(Opcode.rd),Value);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,FALSE, 0);
				}
			}
		}
	} else if (IsKnown(Opcode.rt) || IsKnown(Opcode.rs)) {
		DWORD KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;

		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				unsigned __int64 Value = MipsReg(KnownReg);
				Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
				AndConstToX86Reg(MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
				AndConstToX86Reg(MipsRegLo(Opcode.rd),(DWORD)Value);
			} else {
				DWORD Value = MipsRegLo(KnownReg);
				Map_GPR_32bit(Section,Opcode.rd,IsSigned(KnownReg),UnknownReg);
				AndConstToX86Reg(MipsRegLo(Opcode.rd),(DWORD)Value);
			}
		} else {
			ProtectGPR(Section,KnownReg);
			if (KnownReg == Opcode.rd) {
				if (Is64Bit(KnownReg)) {
					Map_GPR_64bit(Section,Opcode.rd,KnownReg);
					AndVariableToX86Reg(&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg],MipsRegHi(Opcode.rd));
					AndVariableToX86Reg(&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg],MipsRegLo(Opcode.rd));
				} else {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(KnownReg),KnownReg);
					AndVariableToX86Reg(&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg],MipsRegLo(Opcode.rd));
				}
			} else {
				if (Is64Bit(KnownReg)) {
					Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
					AndX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(KnownReg));
					AndX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(KnownReg));
				} else {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(KnownReg),UnknownReg);
					AndX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(KnownReg));
				}
			}
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		AndVariableToX86Reg(&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs],MipsRegHi(Opcode.rd));
		AndVariableToX86Reg(&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs],MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_OR (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (IsKnown(Opcode.rt) && IsKnown(Opcode.rs)) {
		if (IsConst(Opcode.rt) && IsConst(Opcode.rs)) {
			if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {				
				MipsReg(Opcode.rd) = 
					(Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt)) |
					(Is64Bit(Opcode.rs)?MipsReg(Opcode.rs):(_int64)MipsRegLo_S(Opcode.rs));
				if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
					MipsRegState(Opcode.rd) = STATE_CONST_32;
				} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
					MipsRegState(Opcode.rd) = STATE_CONST_32;
				} else {
					MipsRegState(Opcode.rd) = STATE_CONST_64;
				}
			} else {
				MipsRegLo(Opcode.rd) = MipsRegLo(Opcode.rt) | MipsRegLo(Opcode.rs);
				MipsRegState(Opcode.rd) = STATE_CONST_32;
			}
		} else if (IsMapped(Opcode.rt) && IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(Section,Opcode.rt);
			ProtectGPR(Section,Opcode.rs);
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				if (Is64Bit(source2)) {
					OrX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
				}
			} else {
				ProtectGPR(Section,source2);
				Map_GPR_32bit(Section,Opcode.rd,TRUE,source1);
			}
			OrX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
		} else {
			DWORD ConstReg = IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				unsigned _int64 Value;

				if (Is64Bit(ConstReg)) {
					Value = MipsReg(ConstReg);
				} else {
					Value = IsSigned(ConstReg)?MipsRegLo_S(ConstReg):MipsRegLo(ConstReg);
				}
				Map_GPR_64bit(Section,Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,MipsRegLo(Opcode.rd));
				}
			} else {
				int Value = MipsRegLo(ConstReg);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, MappedReg);
				if (Value != 0) { OrConstToX86Reg(Value,MipsRegLo(Opcode.rd)); }
			}
		}
	} else if (IsKnown(Opcode.rt) || IsKnown(Opcode.rs)) {
		int KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = Is64Bit(KnownReg)?MipsReg(KnownReg):MipsRegLo_S(KnownReg);
			Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,MipsRegLo(Opcode.rd));
			}
		} else {
			Map_GPR_64bit(Section,Opcode.rd,KnownReg);
			OrVariableToX86Reg(&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg],MipsRegHi(Opcode.rd));
			OrVariableToX86Reg(&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg],MipsRegLo(Opcode.rd));
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		OrVariableToX86Reg(&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs],MipsRegHi(Opcode.rd));
		OrVariableToX86Reg(&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs],MipsRegLo(Opcode.rd));
	}
	if (SPHack && Opcode.rd == 29) { 
		int count;

		for (count = 0; count < 10; count ++) { x86Protected(count) = FALSE; }
		ResetMemoryStack(Section); 
	}

}

void Compile_R4300i_SPECIAL_XOR (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (Opcode.rt == Opcode.rs) {
		UnMap_GPR(Section, Opcode.rd, FALSE);
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		MipsRegLo(Opcode.rd) = 0;
		return;
	}
	if (IsKnown(Opcode.rt) && IsKnown(Opcode.rs)) {
		if (IsConst(Opcode.rt) && IsConst(Opcode.rs)) {
			if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("XOR 1");
#endif
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				MipsRegState(Opcode.rd) = STATE_CONST_32;
				MipsRegLo(Opcode.rd) = MipsRegLo(Opcode.rt) ^ MipsRegLo(Opcode.rs);
			}
		} else if (IsMapped(Opcode.rt) && IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(Section,source1);
			ProtectGPR(Section,source2);
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				if (Is64Bit(source2)) {
					XorX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(source2));
				} else if (IsSigned(source2)) {
					XorX86RegToX86Reg(MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
				}
				XorX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
			} else {
				if (IsSigned(Opcode.rt) != IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(Opcode.rt),source1);
				}
				XorX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
			}
		} else {
			DWORD ConstReg = IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				DWORD ConstHi, ConstLo;

				ConstHi = Is32Bit(ConstReg)?(DWORD)(MipsRegLo_S(ConstReg) >> 31):MipsRegHi(ConstReg);
				ConstLo = MipsRegLo(ConstReg);
				Map_GPR_64bit(Section,Opcode.rd,MappedReg);
				if (ConstHi != 0) { XorConstToX86Reg(MipsRegHi(Opcode.rd),ConstHi); }
				if (ConstLo != 0) { XorConstToX86Reg(MipsRegLo(Opcode.rd),ConstLo); }
			} else {
				int Value = MipsRegLo(ConstReg);
				if (IsSigned(Opcode.rt) != IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { XorConstToX86Reg(MipsRegLo(Opcode.rd),Value); }
			}
		}
	} else if (IsKnown(Opcode.rt) || IsKnown(Opcode.rs)) {
		int KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned _int64 Value;

			if (Is64Bit(KnownReg)) {
				Value = MipsReg(KnownReg);
			} else {
				if (IsSigned(KnownReg)) {
					Value = (int)MipsRegLo(KnownReg);
				} else {
					Value = MipsRegLo(KnownReg);
				}
			}
			Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				XorConstToX86Reg(MipsRegHi(Opcode.rd),(DWORD)(Value >> 32));
			}
			if ((DWORD)Value != 0) {
				XorConstToX86Reg(MipsRegLo(Opcode.rd),(DWORD)Value);
			}
		} else {
			Map_GPR_64bit(Section,Opcode.rd,KnownReg);
			XorVariableToX86reg(&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg],MipsRegHi(Opcode.rd));
			XorVariableToX86reg(&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg],MipsRegLo(Opcode.rd));
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		XorVariableToX86reg(&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs],MipsRegHi(Opcode.rd));
		XorVariableToX86reg(&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs],MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_NOR (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (IsKnown(Opcode.rt) && IsKnown(Opcode.rs)) {
		if (IsConst(Opcode.rt) && IsConst(Opcode.rs)) {
			if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {				
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				MipsRegLo(Opcode.rd) = ~(MipsRegLo(Opcode.rt) | MipsRegLo(Opcode.rs));
				MipsRegState(Opcode.rd) = STATE_CONST_32;
			}
		} else if (IsMapped(Opcode.rt) && IsMapped(Opcode.rs)) {
			int source1 = Opcode.rd == Opcode.rt?Opcode.rt:Opcode.rs;
			int source2 = Opcode.rd == Opcode.rt?Opcode.rs:Opcode.rt;
			
			ProtectGPR(Section,source1);
			ProtectGPR(Section,source2);
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				Map_GPR_64bit(Section,Opcode.rd,source1);
				if (Is64Bit(source2)) {
					OrX86RegToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(source2));
				} else {
					OrX86RegToX86Reg(MipsRegHi(Opcode.rd),Map_TempReg(Section,x86_Any,source2,TRUE));
				}
				OrX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
				NotX86Reg(MipsRegHi(Opcode.rd));
				NotX86Reg(MipsRegLo(Opcode.rd));
			} else {
				ProtectGPR(Section,source2);
				if (IsSigned(Opcode.rt) != IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE,source1);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(Opcode.rt),source1);
				}
				OrX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(source2));
				NotX86Reg(MipsRegLo(Opcode.rd));
			}
		} else {
			DWORD ConstReg = IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
			DWORD MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				unsigned _int64 Value;

				if (Is64Bit(ConstReg)) {
					Value = MipsReg(ConstReg);
				} else {
					Value = IsSigned(ConstReg)?MipsRegLo_S(ConstReg):MipsRegLo(ConstReg);
				}
				Map_GPR_64bit(Section,Opcode.rd,MappedReg);
				if ((Value >> 32) != 0) {
					OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(Opcode.rd));
				}
				if ((DWORD)Value != 0) {
					OrConstToX86Reg((DWORD)Value,MipsRegLo(Opcode.rd));
				}
				NotX86Reg(MipsRegHi(Opcode.rd));
				NotX86Reg(MipsRegLo(Opcode.rd));
			} else {
				int Value = MipsRegLo(ConstReg);
				if (IsSigned(Opcode.rt) != IsSigned(Opcode.rs)) {
					Map_GPR_32bit(Section,Opcode.rd,TRUE, MappedReg);
				} else {
					Map_GPR_32bit(Section,Opcode.rd,IsSigned(MappedReg)?TRUE:FALSE, MappedReg);
				}
				if (Value != 0) { OrConstToX86Reg(Value,MipsRegLo(Opcode.rd)); }
				NotX86Reg(MipsRegLo(Opcode.rd));
			}
		}
	} else if (IsKnown(Opcode.rt) || IsKnown(Opcode.rs)) {
		int KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		int UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		
		if (IsConst(KnownReg)) {
			unsigned _int64 Value;

			Value = Is64Bit(KnownReg)?MipsReg(KnownReg):MipsRegLo_S(KnownReg);
			Map_GPR_64bit(Section,Opcode.rd,UnknownReg);
			if ((Value >> 32) != 0) {
				OrConstToX86Reg((DWORD)(Value >> 32),MipsRegHi(Opcode.rd));
			}
			if ((DWORD)Value != 0) {
				OrConstToX86Reg((DWORD)Value,MipsRegLo(Opcode.rd));
			}
		} else {
			Map_GPR_64bit(Section,Opcode.rd,KnownReg);
			OrVariableToX86Reg(&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg],MipsRegHi(Opcode.rd));
			OrVariableToX86Reg(&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg],MipsRegLo(Opcode.rd));
		}
		NotX86Reg(MipsRegHi(Opcode.rd));
		NotX86Reg(MipsRegLo(Opcode.rd));
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
		OrVariableToX86Reg(&GPR[Opcode.rs].W[1],GPR_NameHi[Opcode.rs],MipsRegHi(Opcode.rd));
		OrVariableToX86Reg(&GPR[Opcode.rs].W[0],GPR_NameLo[Opcode.rs],MipsRegLo(Opcode.rd));
		NotX86Reg(MipsRegHi(Opcode.rd));
		NotX86Reg(MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_SLT (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (IsKnown(Opcode.rt) && IsKnown(Opcode.rs)) {
		if (IsConst(Opcode.rt) && IsConst(Opcode.rs)) {
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				DisplayError("1");
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
				MipsRegState(Opcode.rd) = STATE_CONST_32;	
				if (MipsRegLo_S(Opcode.rs) < MipsRegLo_S(Opcode.rt)) {
					MipsRegLo(Opcode.rd) = 1;
				} else {
					MipsRegLo(Opcode.rd) = 0;
				}
			}
		} else if (IsMapped(Opcode.rt) && IsMapped(Opcode.rs)) {
			ProtectGPR(Section,Opcode.rt);
			ProtectGPR(Section,Opcode.rs);
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					Is64Bit(Opcode.rs)?MipsRegHi(Opcode.rs):Map_TempReg(Section,x86_Any,Opcode.rs,TRUE), 
					Is64Bit(Opcode.rt)?MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				SetlVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs), MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
			} else {
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs), MipsRegLo(Opcode.rt));

				if (MipsRegLo(Opcode.rd) > x86_EDX) {
					SetlVariable(&BranchCompare,"BranchCompare");
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
				} else {					
					Setl(MipsRegLo(Opcode.rd));
					AndConstToX86Reg(MipsRegLo(Opcode.rd), 1);
				}
			}
		} else {
			DWORD ConstReg  = IsConst(Opcode.rs)?Opcode.rs:Opcode.rt;
			DWORD MappedReg = IsConst(Opcode.rs)?Opcode.rt:Opcode.rs;

			ProtectGPR(Section,MappedReg);
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompConstToX86reg(
					Is64Bit(MappedReg)?MipsRegHi(MappedReg):Map_TempReg(Section,x86_Any,MappedReg,TRUE), 
					Is64Bit(ConstReg)?MipsRegHi(ConstReg):(MipsRegLo_S(ConstReg) >> 31)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				if (MappedReg == Opcode.rs) {
					SetlVariable(&BranchCompare,"BranchCompare");
				} else {
					SetgVariable(&BranchCompare,"BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompConstToX86reg(MipsRegLo(MappedReg), MipsRegLo(ConstReg));
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
			} else {
				DWORD Constant = MipsRegLo(ConstReg);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				CompConstToX86reg(MipsRegLo(MappedReg), Constant);
			
				if (MipsRegLo(Opcode.rd) > x86_EDX) {
					if (MappedReg == Opcode.rs) {
						SetlVariable(&BranchCompare,"BranchCompare");
					} else {
						SetgVariable(&BranchCompare,"BranchCompare");
					}
					MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
				} else {					
					if (MappedReg == Opcode.rs) {
						Setl(MipsRegLo(Opcode.rd));
					} else {
						Setg(MipsRegLo(Opcode.rd));
					}
					AndConstToX86Reg(MipsRegLo(Opcode.rd), 1);
				}
			}
		}		
	} else if (IsKnown(Opcode.rt) || IsKnown(Opcode.rs)) {
		DWORD KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		BYTE *Jump[2];
			
		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				CompConstToVariable(((int)MipsRegLo(KnownReg) >> 31),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}
		} else {
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				ProtectGPR(Section,KnownReg);
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		if (KnownReg == (IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetgVariable(&BranchCompare,"BranchCompare");
		} else {
			SetlVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		if (IsConst(KnownReg)) {
			CompConstToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		} else {
			CompX86regToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		}
		if (KnownReg == (IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&GPR[Opcode.rt].W[1],GPR_NameHi[Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetlVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(Section,x86Reg,Opcode.rs,FALSE),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_SLTU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsKnown(Opcode.rt) && IsKnown(Opcode.rs)) {
		if (IsConst(Opcode.rt) && IsConst(Opcode.rs)) {
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
#ifndef EXTERNAL_RELEASE
				DisplayError("1");
#endif
				Compile_R4300i_UnknownOpcode(Section);
			} else {
				if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
				MipsRegState(Opcode.rd) = STATE_CONST_32;	
				if (MipsRegLo(Opcode.rs) < MipsRegLo(Opcode.rt)) {
					MipsRegLo(Opcode.rd) = 1;
				} else {
					MipsRegLo(Opcode.rd) = 0;
				}
			}
		} else if (IsMapped(Opcode.rt) && IsMapped(Opcode.rs)) {
			ProtectGPR(Section,Opcode.rt);
			ProtectGPR(Section,Opcode.rs);
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				BYTE *Jump[2];

				CompX86RegToX86Reg(
					Is64Bit(Opcode.rs)?MipsRegHi(Opcode.rs):Map_TempReg(Section,x86_Any,Opcode.rs,TRUE), 
					Is64Bit(Opcode.rt)?MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE)
				);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				SetbVariable(&BranchCompare,"BranchCompare");
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
				
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs), MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
			} else {
				CompX86RegToX86Reg(MipsRegLo(Opcode.rs), MipsRegLo(Opcode.rt));
				SetbVariable(&BranchCompare,"BranchCompare");
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
			}
		} else {
			if (Is64Bit(Opcode.rt) || Is64Bit(Opcode.rs)) {
				DWORD MappedRegHi, MappedRegLo, ConstHi, ConstLo, MappedReg, ConstReg;
				BYTE *Jump[2];

				ConstReg  = IsConst(Opcode.rt)?Opcode.rt:Opcode.rs;
				MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;
				
				ConstLo = MipsRegLo(ConstReg);
				ConstHi = (int)ConstLo >> 31;
				if (Is64Bit(ConstReg)) { ConstHi = MipsRegHi(ConstReg); }

				ProtectGPR(Section,MappedReg);
				MappedRegLo = MipsRegLo(MappedReg);
				MappedRegHi = MipsRegHi(MappedReg);
				if (Is32Bit(MappedReg)) {
					MappedRegHi = Map_TempReg(Section,x86_Any,MappedReg,TRUE);
				}

		
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				CompConstToX86reg(MappedRegHi, ConstHi);
				JeLabel8("Low Compare",0);
				Jump[0] = RecompPos - 1;
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				JmpLabel8("Continue",0);
				Jump[1] = RecompPos - 1;
	
				CPU_Message("");
				CPU_Message("      Low Compare:");
				*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
				CompConstToX86reg(MappedRegLo, ConstLo);
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				CPU_Message("");
				CPU_Message("      Continue:");
				*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
			} else {
				DWORD Const = IsConst(Opcode.rs)?MipsRegLo(Opcode.rs):MipsRegLo(Opcode.rt);
				DWORD MappedReg = IsConst(Opcode.rt)?Opcode.rs:Opcode.rt;

				CompConstToX86reg(MipsRegLo(MappedReg), Const);
				if (MappedReg == Opcode.rs) {
					SetbVariable(&BranchCompare,"BranchCompare");
				} else {
					SetaVariable(&BranchCompare,"BranchCompare");
				}
				Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
				MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
			}
		}		
	} else if (IsKnown(Opcode.rt) || IsKnown(Opcode.rs)) {
		DWORD KnownReg = IsKnown(Opcode.rt)?Opcode.rt:Opcode.rs;
		DWORD UnknownReg = IsKnown(Opcode.rt)?Opcode.rs:Opcode.rt;
		BYTE *Jump[2];
			
		if (IsConst(KnownReg)) {
			if (Is64Bit(KnownReg)) {
				CompConstToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				CompConstToVariable(((int)MipsRegLo(KnownReg) >> 31),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}
		} else {
			if (Is64Bit(KnownReg)) {
				CompX86regToVariable(MipsRegHi(KnownReg),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			} else {
				ProtectGPR(Section,KnownReg);
				CompX86regToVariable(Map_TempReg(Section,x86_Any,KnownReg,TRUE),&GPR[UnknownReg].W[1],GPR_NameHi[UnknownReg]);
			}			
		}
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		if (KnownReg == (IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
	
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		if (IsConst(KnownReg)) {
			CompConstToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		} else {
			CompX86regToVariable(MipsRegLo(KnownReg),&GPR[UnknownReg].W[0],GPR_NameLo[UnknownReg]);
		}
		if (KnownReg == (IsConst(KnownReg)?Opcode.rs:Opcode.rt)) {
			SetaVariable(&BranchCompare,"BranchCompare");
		} else {
			SetbVariable(&BranchCompare,"BranchCompare");
		}
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
	} else {
		BYTE *Jump[2];
		int x86Reg;			

		x86Reg = Map_TempReg(Section,x86_Any,Opcode.rs,TRUE);
		CompX86regToVariable(x86Reg,&GPR[Opcode.rt].W[1],GPR_NameHi[Opcode.rt]);
		JeLabel8("Low Compare",0);
		Jump[0] = RecompPos - 1;
		SetbVariable(&BranchCompare,"BranchCompare");
		JmpLabel8("Continue",0);
		Jump[1] = RecompPos - 1;
		
		CPU_Message("");
		CPU_Message("      Low Compare:");
		*((BYTE *)(Jump[0]))=(BYTE)(RecompPos - Jump[0] - 1);
		CompX86regToVariable(Map_TempReg(Section,x86Reg,Opcode.rs,FALSE),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
		SetbVariable(&BranchCompare,"BranchCompare");
		CPU_Message("");
		CPU_Message("      Continue:");
		*((BYTE *)(Jump[1]))=(BYTE)(RecompPos - Jump[1] - 1);
		Map_GPR_32bit(Section,Opcode.rd,TRUE, -1);
		MoveVariableToX86reg(&BranchCompare,"BranchCompare",MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_DADD (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(Opcode.rt)  && IsConst(Opcode.rs)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsReg(Opcode.rd) = 
			Is64Bit(Opcode.rs)?MipsReg(Opcode.rs):(_int64)MipsRegLo_S(Opcode.rs) +
			Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt);		
		if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
		if (IsConst(Opcode.rt)) {
			AddConstToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			AddConstToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(Opcode.rt));
		} else if (IsMapped(Opcode.rt)) {
			int HiReg = Is64Bit(Opcode.rt)?MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(Section,Opcode.rt);
			AddX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			AdcX86RegToX86Reg(MipsRegHi(Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(MipsRegLo(Opcode.rd),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
			AdcVariableToX86reg(MipsRegHi(Opcode.rd),&GPR[Opcode.rt].W[1],GPR_NameHi[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_DADDU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(Opcode.rt)  && IsConst(Opcode.rs)) {
		__int64 ValRs = Is64Bit(Opcode.rs)?MipsReg(Opcode.rs):(_int64)MipsRegLo_S(Opcode.rs);
		__int64 ValRt = Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt);
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsReg(Opcode.rd) = ValRs + ValRt;
		if ((MipsRegHi(Opcode.rd) == 0) && (MipsRegLo(Opcode.rd) & 0x80000000) == 0) {
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if ((MipsRegHi(Opcode.rd) == 0xFFFFFFFF) && (MipsRegLo(Opcode.rd) & 0x80000000) != 0) {
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}
	} else {
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
		if (IsConst(Opcode.rt)) {
			AddConstToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			AddConstToX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(Opcode.rt));
		} else if (IsMapped(Opcode.rt)) {
			int HiReg = Is64Bit(Opcode.rt)?MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(Section,Opcode.rt);
			AddX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			AdcX86RegToX86Reg(MipsRegHi(Opcode.rd),HiReg);
		} else {
			AddVariableToX86reg(MipsRegLo(Opcode.rd),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
			AdcVariableToX86reg(MipsRegHi(Opcode.rd),&GPR[Opcode.rt].W[1],GPR_NameHi[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_DSUB (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(Opcode.rt)  && IsConst(Opcode.rs)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsReg(Opcode.rd) = 
			Is64Bit(Opcode.rs)?MipsReg(Opcode.rs):(_int64)MipsRegLo_S(Opcode.rs) -
			Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt);		
		if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}
	} else {
		if (Opcode.rd == Opcode.rt) {
			int HiReg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			int LoReg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),LoReg);
			SbbX86RegToX86Reg(MipsRegHi(Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
		if (IsConst(Opcode.rt)) {
			SubConstFromX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			SbbConstFromX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(Opcode.rt));
		} else if (IsMapped(Opcode.rt)) {
			int HiReg = Is64Bit(Opcode.rt)?MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(Section,Opcode.rt);
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			SbbX86RegToX86Reg(MipsRegHi(Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(MipsRegLo(Opcode.rd),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
			SbbVariableFromX86reg(MipsRegHi(Opcode.rd),&GPR[Opcode.rt].W[1],GPR_NameHi[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_DSUBU (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (Opcode.rd == 0) { return; }

	if (IsConst(Opcode.rt)  && IsConst(Opcode.rs)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsReg(Opcode.rd) = 
			Is64Bit(Opcode.rs)?MipsReg(Opcode.rs):(_int64)MipsRegLo_S(Opcode.rs) -
			Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt);		
		if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}
	} else {
		if (Opcode.rd == Opcode.rt) {
			int HiReg = Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			int LoReg = Map_TempReg(Section,x86_Any,Opcode.rt,FALSE);
			Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),LoReg);
			SbbX86RegToX86Reg(MipsRegHi(Opcode.rd),HiReg);
			return;
		}
		Map_GPR_64bit(Section,Opcode.rd,Opcode.rs);
		if (IsConst(Opcode.rt)) {
			SubConstFromX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			SbbConstFromX86Reg(MipsRegHi(Opcode.rd),MipsRegHi(Opcode.rt));
		} else if (IsMapped(Opcode.rt)) {
			int HiReg = Is64Bit(Opcode.rt)?MipsRegHi(Opcode.rt):Map_TempReg(Section,x86_Any,Opcode.rt,TRUE);
			ProtectGPR(Section,Opcode.rt);
			SubX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rt));
			SbbX86RegToX86Reg(MipsRegHi(Opcode.rd),HiReg);
		} else {
			SubVariableFromX86reg(MipsRegLo(Opcode.rd),&GPR[Opcode.rt].W[0],GPR_NameLo[Opcode.rt]);
			SbbVariableFromX86reg(MipsRegHi(Opcode.rd),&GPR[Opcode.rt].W[1],GPR_NameHi[Opcode.rt]);
		}
	}
}

void Compile_R4300i_SPECIAL_DSLL (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (IsConst(Opcode.rt)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }

		MipsReg(Opcode.rd) = Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt) << Opcode.sa;
		if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}
		return;
	}
	
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	ShiftLeftDoubleImmed(MipsRegHi(Opcode.rd),MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
	ShiftLeftSignImmed(	MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_DSRL (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (IsConst(Opcode.rt)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }

		MipsReg(Opcode.rd) = Is64Bit(Opcode.rt)?MipsReg(Opcode.rt):(QWORD)MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	ShiftRightDoubleImmed(MipsRegLo(Opcode.rd),MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
	ShiftRightUnsignImmed(MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_DSRA (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (IsConst(Opcode.rt)) {
		if (IsMapped(Opcode.rd)) { UnMap_GPR(Section,Opcode.rd, FALSE); }

		MipsReg_S(Opcode.rd) = Is64Bit(Opcode.rt)?MipsReg_S(Opcode.rt):(_int64)MipsRegLo_S(Opcode.rt) >> Opcode.sa;
		if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}
		return;
	}	
	Map_GPR_64bit(Section,Opcode.rd,Opcode.rt);
	ShiftRightDoubleImmed(MipsRegLo(Opcode.rd),MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
	ShiftRightSignImmed(MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
}

void Compile_R4300i_SPECIAL_DSLL32 (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	if (Opcode.rd == 0) { return; }
	if (IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegHi(Opcode.rd) = MipsRegLo(Opcode.rt) << Opcode.sa;
		MipsRegLo(Opcode.rd) = 0;
		if (MipsRegLo_S(Opcode.rd) < 0 && MipsRegHi_S(Opcode.rd) == -1){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else if (MipsRegLo_S(Opcode.rd) >= 0 && MipsRegHi_S(Opcode.rd) == 0){ 
			MipsRegState(Opcode.rd) = STATE_CONST_32;
		} else {
			MipsRegState(Opcode.rd) = STATE_CONST_64;
		}

	} else if (IsMapped(Opcode.rt)) {
		ProtectGPR(Section,Opcode.rt);
		Map_GPR_64bit(Section,Opcode.rd,-1);		
		if (Opcode.rt != Opcode.rd) {
			MoveX86RegToX86Reg(MipsRegLo(Opcode.rt),MipsRegHi(Opcode.rd));
		} else {
			int HiReg = MipsRegHi(Opcode.rt);
			MipsRegHi(Opcode.rt) = MipsRegLo(Opcode.rt);
			MipsRegLo(Opcode.rt) = HiReg;
		}
		if ((BYTE)Opcode.sa != 0) {
			ShiftLeftSignImmed(MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
		}
		XorX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rd));
	} else {
		Map_GPR_64bit(Section,Opcode.rd,-1);
		MoveVariableToX86reg(&GPR[Opcode.rt],GPR_NameHi[Opcode.rt],MipsRegHi(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftLeftSignImmed(MipsRegHi(Opcode.rd),(BYTE)Opcode.sa);
		}
		XorX86RegToX86Reg(MipsRegLo(Opcode.rd),MipsRegLo(Opcode.rd));
	}
}

void Compile_R4300i_SPECIAL_DSRL32 (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		MipsRegLo(Opcode.rd) = (DWORD)(MipsReg(Opcode.rt) >> (Opcode.sa + 32));
	} else if (IsMapped(Opcode.rt)) {
		ProtectGPR(Section,Opcode.rt);
		if (Is64Bit(Opcode.rt)) {
			if (Opcode.rt == Opcode.rd) {
				int HiReg = MipsRegHi(Opcode.rt);
				MipsRegHi(Opcode.rt) = MipsRegLo(Opcode.rt);
				MipsRegLo(Opcode.rt) = HiReg;
				Map_GPR_32bit(Section,Opcode.rd,FALSE,-1);
			} else {
				Map_GPR_32bit(Section,Opcode.rd,FALSE,-1);
				MoveX86RegToX86Reg(MipsRegHi(Opcode.rt),MipsRegLo(Opcode.rd));
			}
			if ((BYTE)Opcode.sa != 0) {
				ShiftRightUnsignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
			}
		} else {
			Compile_R4300i_UnknownOpcode(Section);
		}
	} else {
		Map_GPR_32bit(Section,Opcode.rd,FALSE,-1);
		MoveVariableToX86reg(&GPR[Opcode.rt].UW[1],GPR_NameLo[Opcode.rt],MipsRegLo(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftRightUnsignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	}
}

void Compile_R4300i_SPECIAL_DSRA32 (BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (IsConst(Opcode.rt)) {
		if (Opcode.rt != Opcode.rd) { UnMap_GPR(Section,Opcode.rd, FALSE); }
		MipsRegState(Opcode.rd) = STATE_CONST_32;
		MipsRegLo(Opcode.rd) = (DWORD)(MipsReg_S(Opcode.rt) >> (Opcode.sa + 32));
	} else if (IsMapped(Opcode.rt)) {
		ProtectGPR(Section,Opcode.rt);
		if (Is64Bit(Opcode.rt)) {
			if (Opcode.rt == Opcode.rd) {
				int HiReg = MipsRegHi(Opcode.rt);
				MipsRegHi(Opcode.rt) = MipsRegLo(Opcode.rt);
				MipsRegLo(Opcode.rt) = HiReg;
				Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
			} else {
				Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
				MoveX86RegToX86Reg(MipsRegHi(Opcode.rt),MipsRegLo(Opcode.rd));
			}
			if ((BYTE)Opcode.sa != 0) {
				ShiftRightSignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
			}
		} else {
			Compile_R4300i_UnknownOpcode(Section);
		}
	} else {
		Map_GPR_32bit(Section,Opcode.rd,TRUE,-1);
		MoveVariableToX86reg(&GPR[Opcode.rt].UW[1],GPR_NameLo[Opcode.rt],MipsRegLo(Opcode.rd));
		if ((BYTE)Opcode.sa != 0) {
			ShiftRightSignImmed(MipsRegLo(Opcode.rd),(BYTE)Opcode.sa);
		}
	}
}

/************************** COP0 functions **************************/
void Compile_R4300i_COP0_MF(BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	switch (Opcode.rd) {
	case 9: //Count
		AddConstToVariable(BlockCycleCount,&CP0[9],Cop0_Name[9]);
		SubConstFromVariable(BlockCycleCount,&Timers.Timer,"Timer");
		BlockCycleCount = 0;
	}
	Map_GPR_32bit(Section,Opcode.rt,TRUE,-1);
	MoveVariableToX86reg(&CP0[Opcode.rd],Cop0_Name[Opcode.rd],MipsRegLo(Opcode.rt));
}

void Compile_R4300i_COP0_MT (BLOCK_SECTION * Section) {
	int OldStatusReg;
	BYTE *Jump;

	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	switch (Opcode.rd) {
	case 0: //Index
	case 2: //EntryLo0
	case 3: //EntryLo1
	case 4: //Context
	case 5: //PageMask
	case 9: //Count
	case 10: //Entry Hi
	case 11: //Compare
	case 14: //EPC
	case 16: //Config
	case 18: //WatchLo 
	case 19: //WatchHi
	case 28: //Tag lo
	case 29: //Tag Hi
	case 30: //ErrEPC
		if (IsConst(Opcode.rt)) {
			MoveConstToVariable(MipsRegLo(Opcode.rt), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regToVariable(MipsRegLo(Opcode.rt), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		}
		switch (Opcode.rd) {
		case 4: //Context
			AndConstToVariable(0xFF800000,&CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
			break;			
		case 9: //Count
			BlockCycleCount = 0;
			Pushad();
			Call_Direct(ChangeCompareTimer,"ChangeCompareTimer");
			Popad();
			break;
		case 11: //Compare
			AddConstToVariable(BlockCycleCount,&CP0[9],Cop0_Name[9]);
			SubConstFromVariable(BlockCycleCount,&Timers.Timer,"Timer");
			BlockCycleCount = 0;
			AndConstToVariable(~CAUSE_IP7,&FAKE_CAUSE_REGISTER,"FAKE_CAUSE_REGISTER");
			Pushad();
			Call_Direct(ChangeCompareTimer,"ChangeCompareTimer");
			Popad();
		}
		break;
	case 12: //Status
		OldStatusReg = Map_TempReg(Section,x86_Any,-1,FALSE);
		MoveVariableToX86reg(&CP0[Opcode.rd],Cop0_Name[Opcode.rd],OldStatusReg);
		if (IsConst(Opcode.rt)) {
			MoveConstToVariable(MipsRegLo(Opcode.rt), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regToVariable(MipsRegLo(Opcode.rt), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		}
		XorVariableToX86reg(&CP0[Opcode.rd], Cop0_Name[Opcode.rd],OldStatusReg);
		TestConstToX86Reg(STATUS_FR,OldStatusReg);
		JeLabel8("FpuFlagFine",0);
		Jump = RecompPos - 1;
		Pushad();
		Call_Direct(SetFpuLocations,"SetFpuLocations");
		Popad();
		*(BYTE *)(Jump)= (BYTE )(((BYTE )(RecompPos)) - (((BYTE )(Jump)) + 1));
				
		//TestConstToX86Reg(STATUS_FR,OldStatusReg);
		//CompileExit(Section->CompilePC+4,Section->RegWorking,ExitResetRecompCode,FALSE,JneLabel32);
		Pushad();
		Call_Direct(CheckInterrupts,"CheckInterrupts");
		Popad();
		break;
	case 6: //Wired
		Pushad();
		if (BlockRandomModifier != 0) { SubConstFromVariable(BlockRandomModifier,&CP0[1],Cop0_Name[1]); }
		BlockRandomModifier = 0;
		Call_Direct(FixRandomReg,"FixRandomReg");
		Popad();
		if (IsConst(Opcode.rt)) {
			MoveConstToVariable(MipsRegLo(Opcode.rt), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		} else if (IsMapped(Opcode.rt)) {
			MoveX86regToVariable(MipsRegLo(Opcode.rt), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		} else {
			MoveX86regToVariable(Map_TempReg(Section,x86_Any,Opcode.rt,FALSE), &CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
		}
		break;
	case 13: //cause
		if (IsConst(Opcode.rt)) {
			AndConstToVariable(0xFFFFCFF,&CP0[Opcode.rd], Cop0_Name[Opcode.rd]);
#ifndef EXTERNAL_RELEASE
			if ((MipsRegLo(Opcode.rt) & 0x300) != 0 ){ DisplayError("Set IP0 or IP1"); }
#endif
		} else {
			Compile_R4300i_UnknownOpcode(Section);
		}
		Pushad();
		Call_Direct(CheckInterrupts,"CheckInterrupts");
		Popad();
		break;
	default:
		Compile_R4300i_UnknownOpcode(Section);
	}
}

/************************** COP0 CO functions ***********************/
void Compile_R4300i_COP0_CO_TLBR( BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (!UseTlb) {	return; }
	Pushad();
	Call_Direct(TLB_Read,"TLB_Read");
	Popad();
}

void Compile_R4300i_COP0_CO_TLBWI( BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (!UseTlb) {	return; }
	Pushad();
	MoveVariableToX86reg(&INDEX_REGISTER,"INDEX_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Call_Direct(WriteTLBEntry,"WriteTLBEntry");
	Popad();
}

void Compile_R4300i_COP0_CO_TLBWR( BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	if (!UseTlb) {	return; }
	if (BlockRandomModifier != 0) { SubConstFromVariable(BlockRandomModifier,&CP0[1],Cop0_Name[1]); }
	BlockRandomModifier = 0;
	Pushad();
	Call_Direct(FixRandomReg,"FixRandomReg");
	MoveVariableToX86reg(&RANDOM_REGISTER,"RANDOM_REGISTER",x86_ECX);
	AndConstToX86Reg(x86_ECX,0x1F);
	Call_Direct(WriteTLBEntry,"WriteTLBEntry");
	Popad();
}

void Compile_R4300i_COP0_CO_TLBP( BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));
	
	if (!UseTlb) {	return; }
	Pushad();
	Call_Direct(TLB_Probe,"TLB_Probe");
	Popad();
}

void compiler_COP0_CO_ERET (void) {
	if ((STATUS_REGISTER & STATUS_ERL) != 0) {
		PROGRAM_COUNTER = ERROREPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		PROGRAM_COUNTER = EPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_EXL;
	}
	LLBit = 0;
	CheckInterrupts();
}

void Compile_R4300i_COP0_CO_ERET( BLOCK_SECTION * Section) {
	CPU_Message("  %X %s",Section->CompilePC,R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	WriteBackRegisters(Section);
	Call_Direct(compiler_COP0_CO_ERET,"compiler_COP0_CO_ERET");
	CompileExit((DWORD)-1,Section->RegWorking,Normal,TRUE,NULL);
	NextInstruction = END_BLOCK;
}

/************************** Other functions **************************/
void Compile_R4300i_UnknownOpcode (BLOCK_SECTION * Section) {
	CPU_Message("  %X Unhandled Opcode: %s",Section->CompilePC, R4300iOpcodeName(Opcode.Hex,Section->CompilePC));

	FreeSection(Section->ContinueSection,Section);
	FreeSection(Section->JumpSection,Section);
	BlockCycleCount -= CountPerOp;
	BlockRandomModifier -= 1;
	MoveConstToVariable(Section->CompilePC,&PROGRAM_COUNTER,"PROGRAM_COUNTER");
	WriteBackRegisters(Section);
	AddConstToVariable(BlockCycleCount,&CP0[9],Cop0_Name[9]);
	SubConstFromVariable(BlockRandomModifier,&CP0[1],Cop0_Name[1]);
	if (CPU_Type == CPU_SyncCores) { Call_Direct(SyncToPC, "SyncToPC"); }
	MoveConstToVariable(Opcode.Hex,&Opcode.Hex,"Opcode.Hex");
	Call_Direct(R4300i_UnknownOpcode, "R4300i_UnknownOpcode");
	Ret();
	if (NextInstruction == NORMAL) { NextInstruction = END_BLOCK; }
}


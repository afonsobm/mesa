/*
 * Copyright © 2018 Valve Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Daniel Schürmann (daniel.schuermann@campus.tu-berlin.de)
 *
 */


#include <unordered_map>
#include <unordered_set>
#include <map>

#include "aco_ir.h"


namespace aco {

struct InstrHash {
   std::size_t operator()(Instruction* instr) const
   {
      uint64_t hash = (uint64_t) instr->opcode + (uint64_t) instr->format;
      for (unsigned i = 0; i < instr->num_operands; i++) {
         Operand op = instr->getOperand(i);
         uint64_t val = op.isTemp() ? op.tempId() : op.isFixed() ? op.physReg().reg : op.constantValue();
         hash |= val << (i+1) * 8;
      }
      if (instr->isVOP3()) {
         VOP3A_instruction* vop3 = static_cast<VOP3A_instruction*>(instr);
         for (unsigned i = 0; i < 3; i++) {
            hash ^= vop3->abs[i] << (i*3 + 0);
            hash ^= vop3->opsel[i] << (i*3 + 1);
            hash ^= vop3->neg[i] << (i*3 + 2);
         }
         hash ^= (vop3->clamp << 28) * 13;
         hash += vop3->omod << 19;
      }
      switch (instr->format) {
      case Format::SMEM:
         break;
      case Format::VINTRP: {
         Interp_instruction* interp = static_cast<Interp_instruction*>(instr);
         hash ^= interp->attribute << 13;
         hash ^= interp->component << 27;
         break;
      }
      case Format::DS:
         break;
      default:
         break;
      }

      return hash;
   }
};

struct InstrPred {
   bool operator()(Instruction* a, Instruction* b) const
   {
      if (a->format != b->format)
         return false;
      if (a->opcode != b->opcode)
         return false;
      for (unsigned i = 0; i < a->num_operands; i++)
         if (a->getOperand(i).constantValue() != b->getOperand(i).constantValue())
            return false;
      if (a->isVOP3()) {
         VOP3A_instruction* a3 = static_cast<VOP3A_instruction*>(a);
         VOP3A_instruction* b3 = static_cast<VOP3A_instruction*>(b);
         for (unsigned i = 0; i < 3; i++) {
            if (a3->abs[i] != b3->abs[i] ||
                a3->opsel[i] != b3->opsel[i] ||
                a3->neg[i] != b3->neg[i])
               return false;
         }
         return a3->clamp == b3->clamp &&
                a3->omod == b3->omod;
      }
      switch (a->format) {
         case Format::SOPK: {
            SOPK_instruction* aK = static_cast<SOPK_instruction*>(a);
            SOPK_instruction* bK = static_cast<SOPK_instruction*>(b);
            return aK->imm == bK->imm;
         }
         case Format::SMEM: {
            SMEM_instruction* aS = static_cast<SMEM_instruction*>(a);
            SMEM_instruction* bS = static_cast<SMEM_instruction*>(b);
            return aS->glc == bS->glc && aS->nv == bS->nv;
         }
         case Format::VINTRP: {
            Interp_instruction* aI = static_cast<Interp_instruction*>(a);
            Interp_instruction* bI = static_cast<Interp_instruction*>(b);
            if (aI->attribute != bI->attribute)
               return false;
            if (aI->component != bI->component)
               return false;
            return true;
         }
         case Format::DS: {
            DS_instruction* aDS = static_cast<DS_instruction*>(a);
            DS_instruction* bDS = static_cast<DS_instruction*>(b);
            return aDS->offset0 == bDS->offset0 &&
                   aDS->offset1 == bDS->offset1 &&
                   aDS->gds == bDS->gds;
         }
         case Format::MUBUF: {
            MUBUF_instruction* aMU = static_cast<MUBUF_instruction*>(a);
            MUBUF_instruction* bMU = static_cast<MUBUF_instruction*>(b);
            return aMU->dfmt == bMU->dfmt &&
                   aMU->nfmt == bMU->nfmt &&
                   aMU->offset == bMU->offset &&
                   aMU->offen == bMU->offen &&
                   aMU->idxen == bMU->idxen &&
                   aMU->glc == bMU->glc &&
                   aMU->slc == bMU->slc &&
                   aMU->tfe == bMU->tfe &&
                   aMU->lds == bMU->lds;
         }
         case Format::MIMG: {
            MIMG_instruction* aMI = static_cast<MIMG_instruction*>(a);
            MIMG_instruction* bMI = static_cast<MIMG_instruction*>(b);
            return aMI->dmask == bMI->dmask &&
                   aMI->unrm == bMI->unrm &&
                   aMI->glc == bMI->glc &&
                   aMI->slc == bMI->slc &&
                   aMI->tfe == bMI->tfe &&
                   aMI->da == bMI->da &&
                   aMI->lwe == bMI->lwe &&
                   aMI->a16 == bMI->a16 &&
                   aMI->d16 == bMI->d16;
         }
         default:
            return true;
      }
   }
};

struct lvn_ctx {
   std::unordered_set<Instruction*, InstrHash, InstrPred> expr_values;
   std::map<uint32_t, Temp> renames;
};


void lookup_instruction(lvn_ctx& ctx, std::unique_ptr<Instruction>& instr)
{
   for (unsigned i = 0; i < instr->num_operands; i++) {
      if (!instr->getOperand(i).isTemp())
         continue;
      std::map<uint32_t, Temp>::iterator it = ctx.renames.find(instr->getOperand(i).tempId());
      if (it != ctx.renames.end())
         instr->getOperand(i) = Operand(it->second);
   }

   if (!instr->num_definitions)
      return;

   std::pair<std::unordered_set<Instruction*, InstrHash, InstrPred>::iterator, bool> res = ctx.expr_values.emplace(instr.get());

   /* if there was already an expression with the same value number */
   if (!res.second) {
      Instruction* orig_instr = *(res.first);
      assert(instr->num_definitions == orig_instr->num_definitions);
      for (unsigned i = 0; i < instr->num_definitions; i++) {
         assert(instr->getDefinition(i).regClass() == orig_instr->getDefinition(i).regClass());
         ctx.renames.emplace(instr->getDefinition(i).tempId(), orig_instr->getDefinition(i).getTemp());
      }
   }
}

void opt_lvn(Program* program)
{
   lvn_ctx ctx;
   for (auto&& block : program->blocks) {
      for (std::unique_ptr<Instruction>& instr : block->instructions)
         lookup_instruction(ctx, instr);
      ctx.expr_values.clear();
   }
}

}

template = """\
% for name in SOP2:
<%
   operands = ['ssrc0']
   if name != 's_rfe_restore_b64':
      operands.append('ssrc1')
   if opcodes[name].num_inputs == 3:
      operands.append('scc')
   type = 'SOP2<'+str(opcodes[name].num_inputs)+','+str(opcodes[name].num_outputs)+'>'
%>
   ${type}*
   ${name}(\\
      % for op in operands:
Operand ${op}${', ' if op != operands[-1] else ')'}\\
      % endfor

   {
      ${type}* instr = new ${type}(aco_opcode::${name});
      % for idx,ty in enumerate(opcodes[name].output_type):
      instr->getDefinition(${idx}) = Definition(P.allocateId(), RegClass::${ty});
      % endfor
      % for idx,op in enumerate(operands):
      instr->getOperand(${idx}) = ${op};
      % endfor
      insertInstruction(instr);
      return instr;
   }
% endfor
% for name in SOPK:
<%
   op = 'scc' if opcodes[name].read_reg == 'SCC' else 'ssrc' if opcodes[name].num_inputs == 1 else ''
   type = 'SOPK<'+ str(opcodes[name].num_inputs) +','+str(opcodes[name].num_outputs)+'>'
%>
   ${type}*
   ${name}(unsigned imm\\
      % if op:
, Operand ${op}\\
      % endif
)
   {
      ${type}* instr = new ${type}(aco_opcode::${name}, imm);
      % for idx,ty in enumerate(opcodes[name].output_type):
      instr->getDefinition(${idx}) = Definition(P.allocateId(), RegClass::${ty});
      % endfor
      % if op:
      instr->getOperand(${0}) = ${op};
      % endif
      % if name == 's_addk_i32' or name == 's_mulk_i32':
      instr->getOperand(0).setKill(true);
      instr->getDefinition(0).setReuseInput(true);
      % endif
      insertInstruction(instr);
      return instr;
   }
% endfor
% for name in SOP1:
<%
   operands = []
   if opcodes[name].num_inputs > 0:
      operands.append('ssrc0')
   if opcodes[name].num_inputs == 2:
      operands.append('scc')
   type = 'SOP1<'+ str(opcodes[name].num_inputs) +','+str(opcodes[name].num_outputs)+'>'
%>
   ${type}*
   ${name}(\\
      % for op in operands:
Operand ${op}${', ' if op != operands[-1] else ''}\\
      % endfor
)
   {
      ${type}* instr = new ${type}(aco_opcode::${name});
      % for idx,ty in enumerate(opcodes[name].output_type):
      instr->getDefinition(${idx}) = Definition(P.allocateId(), RegClass::${ty});
      % endfor
      % for idx,op in enumerate(operands):
      instr->getOperand(${idx}) = ${op};
      % endfor
      insertInstruction(instr);
      return instr;
   }
% endfor
## TODO: SOPC
% for name in SOPP:
<%
   op = 'scc' if opcodes[name].read_reg == 'SCC' else 'vcc' if opcodes[name].read_reg == 'VCC' else ''
   type = 'SOPP<'+ str(opcodes[name].num_inputs) +','+str(opcodes[name].num_outputs)+'>'
%>
   ${type}*
   ${name}(\\
      % if op:
Operand ${op}\\
      % endif
)
   {
      ${type}* instr = new ${type}(aco_opcode::${name});
      % if op:
      instr->getOperand(${0}) = ${op};
      % endif
      insertInstruction(instr);
      return instr;
   }
% endfor
## TODO: SMEM
## TODO: VOP2
% for name in VOP1:
<%
   operands = []
   if opcodes[name].num_inputs > 0:
      operands.append('src0')
   if opcodes[name].num_inputs == 2:
      operands.append('src1')
   type = 'VOP1<'+ str(opcodes[name].num_inputs) +','+str(opcodes[name].num_outputs)+'>'
%>
   ${type}*
   ${name}(\\
      % for op in operands:
Operand ${op}${', ' if op != operands[-1] else ''}\\
      % endfor
)
   {
      ${type}* instr = new ${type}(aco_opcode::${name});
      % for idx,ty in enumerate(opcodes[name].output_type):
      instr->getDefinition(${idx}) = Definition(P.allocateId(), RegClass::${ty});
      % endfor
      % for idx,op in enumerate(operands):
      instr->getOperand(${idx}) = ${op};
      % endfor
      % if name == 'v_swap_b32':
      instr->getOperand(0).setKill(true);
      instr->getDefinition(0).setReuseInput(true);
      instr->getOperand(1).setKill(true);
      instr->getDefinition(1).setReuseInput(true);
      % endif
      insertInstruction(instr);
      return instr;
   }
% endfor
"""

import aco_opcodes
from aco_opcodes import opcodes
from mako.template import Template

print Template(template).render(
   opcodes=opcodes,
   SOP2=aco_opcodes.SOP2,
   SOPK=aco_opcodes.SOPK,
   SOP1=aco_opcodes.SOP1,
   SOPP=aco_opcodes.SOPP,
   VOP1=aco_opcodes.VOP1)
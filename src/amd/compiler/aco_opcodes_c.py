
template = """\
/* 
 * Copyright (c) 2018 Valve Corporation
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
 *    Daniel Schuermann (daniel.schuermann@campus.tu-berlin.de)
 */

#include "aco_ir.h"

const opcode_info opcode_infos[num_opcodes] = {
% for name, opcode in sorted(opcodes.iteritems()):
{
   .name = "${name}",
   .num_inputs = ${opcode.num_inputs},
   .num_outputs = ${opcode.num_outputs},
   .output_type = { ${ ", ".join(str(type) for type in opcode.output_type) }},
   .kills_input = {${ ", ".join(str(size) for size in opcode.kills_input) }}
},
% endfor
};
"""

from aco_opcodes import opcodes
from mako.template import Template

print Template(template).render(opcodes=opcodes)
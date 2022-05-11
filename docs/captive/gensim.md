## [Gensim](https://github.com/gensim-project/gensim)
-  [ ] How the data transfer from


ISADescriptionParser::ParseFile
  - > ArchDescriptionParser::load_arch_ctor
    - > ArchDescriptionParser::load_from_arch_node
      - > ArchDescriptionParser::ParseFile
        - > main : called by gensim/tools
  - > ISADescriptionParser::load_from_node
    - > ISADescriptionParser::ParseFile


- [x] so, how gensim is used for by ArcSim

```plain
------------------------------------------------------------------------
1.0 The Gensim Compiler
------------------------------------------------------------------------

The Gensim Compiler reads in Gensim ADL models, performs some
processing, and outputs (generally) C++ source code.

A Gensim model consists of several main components:
1. The System description, which contains information on register banks,
   features, etc.
2. The Syntactic description, which specifies how instructions are
   encoded
3. The Semantic description, which specifies how instructions are
   executed.

Each of these components is processed by a different part of the
toolchain. The most complex processing is performed on the Semantic
component (which is also usually the largest component).


If you are interested in writing a model or using gensim, you should
read the documentation in the models directory. The documentation in
this directory is more intended for developers.
```

- [ ] gensim/gensim/src/arch and gensim/gensim/src/isa


cmake trick:
1. https://stackoverflow.com/questions/26243169/cmake-target-include-directories-meaning-of-scope
2. /home/maritns3/core/captive-project/gensim/gensim/src/isa/ISADescriptionParser.cpp
```c
#include <archcasm/archcasmParser.h>
#include <archcasm/archcasmLexer.h>
#include <archCBehaviour/archCBehaviourLexer.h>
#include <archCBehaviour/archCBehaviourParser.h>
```


## code flow
- [ ] ArchDescriptionParser seems have a similar way

- [ ] why we can generate several C++ file
- [ ] What if we can't

- ISADescriptionParser::ParseFile : build the syntax tree
  - archcLexerNew
  - antlr3CommonTokenStreamSourceNew
  - archcParserNew
  - `parser->arch_isa`
  - antlr3CommonTreeNodeStreamNewTree
  - load_from_node

- main
  - section One : prase files
    - `arch::ArchDescription &description = *parser.Get();`
  - section Two : isa
    - `isa->BuildSSAContext`
    - `isa->GetSSAContext().Optimise();`
    - `isa->GetSSAContext().Resolve(root_context);`
  - section three : component

```plain
COMMAND ${gensim-binary} -a ${CMAKE_CURRENT_SOURCE_DIR}/${arch-file} ${gensim-options} -t ${CMAKE_CURRENT_BINARY_DIR}/output-${arch-name}/
```

```plain
function(define_model target-name arch-name arch-file)
-- armv7a------------------<<<<<
-- model-armv7a------------<<<<
-- armv7a.ac---------------<<<<
```
armv7a.ac is root file, other files will be imported



ArchDescription contains multiple ISADescription.
## src/grammar
- [ ] genC.g
  - It's cool, generate genC to change data

it's related to : /home/maritns3/core/captive-project/gensim/gensim/src/genC

- [ ] what's relation with generator and genC

## src/generator
Of course, there is generator for interpreter, blockjit and LLVM.

- [ ] I guess LLVM is not finished.

## src/isa
ISADescriptionParser::load_isa_from_node
  - AsmDescriptionParser::Parse

After reading the files one by one, I think it's just a praser.

## class ISADescription
1. relation with ISADescriptionParser
  - ISADescriptionParser read a file for ISA : `*_isa.ac`
  - maybe read behavior file



### behavior file
```c
vfpv4.ac
Load Behaviour file decode.vfpv4
Load Behaviour file execute.vfpv4
neon.ac
Load Behaviour file decode.vfpv4
Load Behaviour file decode.neon
Load Behaviour file execute.vfpv4
Load Behaviour file execute.neon
armv7a_isa.ac
Load Behaviour file decode.vfpv4
Load Behaviour file behaviours.arm
Load Behaviour file decode.neon
Load Behaviour file execute.vfpv4
Load Behaviour file execute.neon
Load Behaviour file execute.arm
armv5e_thumb_isa.ac
Load Behaviour file behaviours.thumb1
Load Behaviour file execute.thumb1
thumb_vfp.ac
Load Behaviour file behaviours.thumb1
Load Behaviour file execute.thumb1
Load Behaviour file execute.thumb_vfp
armv7a_thumb_isa.ac
Load Behaviour file behaviours.thumb1
Load Behaviour file behaviours.thumb2
Load Behaviour file execute.thumb1
Load Behaviour file execute.thumb_vfp
Load Behaviour file execute.thumb2
```

behaviour works alike C language.

- [ ] /home/maritns3/core/captive-project/gensim/models/armv7/execute.arm : So how execute.arm prased ?
```c
execute(ldrt2)
{
	uint8 c;
	uint32 imm32 = decode_imm((inst.shift_type), (inst.shift_amt), read_gpr(inst.rm), read_register(C), c);
	uint32 addr = read_gpr(inst.rn);
	uint32 offset = inst.u ? addr + imm32 : addr - imm32;

	addr = (inst.rn == 15) ? addr & (uint32)0xfffffffc : addr;

	if(memory_read_32_user(addr, imm32))
	{
		imm32 = (inst.rd == 15) ? imm32 & (uint32)0xfffffffe : imm32;

		if (inst.w | !(inst.p))
		{
			write_register_bank(RB, inst.rn, offset);
		}

		write_register_bank(RB, inst.rd, imm32);
	}
}
```

behaviours ===> How it works ?



/home/maritns3/core/captive-project/gensim/gensim/src/isa/ISADescriptionParser.cpp

```cpp
        std::cout << format << " -- " << code << std::endl;
```


```plain
thumb2_subwi -- {
        if (!(inst.rn == 0xf))
        {
        uint32 n = read_register_bank(RB, inst.rn) + pc_check(inst.rn);
        uint32 imm32 = ((uint32)inst.i << 11) | ((uint32)inst.imm3 << 8) | (uint32)inst.imm8;

        uint8 carry_out;
        uint8 overflow;

        uint32 result = AddWithCarry(n, ~imm32, 1, carry_out, overflow);
        write_register_bank(RB, inst.rd, result);
        }
        else
        {
                trap();
        }
}
```
So, it will furture too something else.


### /home/maritns3/core/captive-project/gensim/gensim/src/isa/ISADescriptionParser.cpp
ISADescriptionParser::load_isa_from_node
  - case AC_MODIFIERS
  - case AC_BEHAVIOURS
  - case AC_DECODES
  - case AC_EXECUTE
  - case SET_DECODER
  - case SET_EOB
  - case SET_USES_PC
  - and many ……

```cpp
			case SET_ASM: {
				pANTLR3_BASE_TREE instrNode = (pANTLR3_BASE_TREE)child->getChild(child, 0);
				std::string instrName = (char *)instrNode->getText(instrNode)->chars;
//				if (Util::Verbose_Level) printf("Loading disasm info for instruction %s\n", instrNode->getText(instrNode)->chars);

				if (isa->Instructions.find(instrName) == isa->Instructions.end()) {
					diag.Error("Attempting to add disasm info to unknown instruction", DiagNode(filename, instrNode));
					success = false;
				} else {
					AsmDescriptionParser asm_parser (diag, filename);
					if(!asm_parser.Parse(child, *isa)) break;
					isa->instructions_[instrName]->Disasms.push_back(asm_parser.Get());
				}
				break;
			}
```

```plain
  ldr2.set_asm("ldr%[cond] %reg, [%reg, +%reg]", cond, rd, rn, rm, p=1, w=0, shift_amt=0, shift_type=0, u=1);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, -%reg]", cond, rd, rn, rm, p=1, w=0, shift_amt=0, shift_type=0, u=0);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, %reg]", cond, rd, rn, rm, p=1, w=0, shift_amt=0, shift_type=0, u=1);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, +%reg, %SHIFTTYPE #%imm]", cond, rd, rn, rm, shift_type, shift_amt, p=1, w=0, u=1);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, -%reg, %SHIFTTYPE #%imm]", cond, rd, rn, rm, shift_type, shift_amt, p=1, w=0, u=0);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, %reg, %SHIFTTYPE #%imm]", cond, rd, rn, rm, shift_type, shift_amt, p=1, w=0, u=1);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, +%reg, rrx]", cond, rd, rn, rm, shift_type=0x03, shift_amt=0x00, p=1, w=0, u=1);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, -%reg, rrx]", cond, rd, rn, rm, shift_type=0x03, shift_amt=0x00, p=1, w=0, u=0);
  ldr2.set_asm("ldr%[cond] %reg, [%reg, %reg, rrx]", cond, rd, rn, rm, shift_type=0x03, shift_amt=0x00, p=1, w=0, u=1);
```

ISADescriptionParser::load_behaviour_file


#### /home/maritns3/core/captive-project/gensim/gensim/src/isa/ISADescription.cpp
ISADescription::BuildSSAContext : called by main, used after tree setup

#### /home/maritns3/core/captive-project/gensim/gensim/src/isa/InstructionFormatDescription.cpp

add one print debug:
```c
bool InstructionFormatDescriptionParser::Parse(const std::string &name, const std::string &format, InstructionFormatDescription *&description)
{
	description = new InstructionFormatDescription(isa_);

  std::cout << name << " --> " << format << std::endl;
```
This is one of the output:

Type_vfpv3_dpi --> %cond:4 0xe:4 %op1:1 %D:1 %op:2 %vn:4 %vd:4 0x5:3 %sz:1 %N:1 %p:1 %M:1 0x0:1 %vm:4

And this is the sourcecode:

/home/maritns3/core/captive-project/gensim/models/armv7/vfpv4.ac
```plain
AC_ISA(arm)
{
	// VFPv3 Instructions
	ac_format Type_vfpv3_dpi = "%cond:4 0xe:4 %op1:1 %D:1 %op:2 %vn:4 %vd:4 0x5:3 %sz:1 %N:1 %p:1 %M:1 0x0:1 %vm:4";
	ac_format Type_vfpv3_dpi_mov = "%cond:4 0xe:4 %op1:1 %D:1 %op:2 %vn:4 %vd:4 0x5:3 %sz:1 %N:1 %p:1 %M:1 0x0:1 %vm:4";
	ac_format Type_vfpv3_cvt    = "%cond:4 0xe:4 0x1:1 %D:1 0x3:2 0x1:1 %opc2:3 %vd:4 0x5:3 %sz:1 %N:1 0x1:1 %M:1 0x0:1 %vm:4";
	ac_format Type_vfpv3_cvt_fxp= "%cond:4 0xe:4 0x1:1 %D:1 0x3:2 0x1:1 %opc2:3 %vd:4 0x5:3 %sz:1 %N:1 0x1:1 %M:1 0x0:1 %vm:4";
```

/home/maritns3/core/captive-project/gensim/models/armv7/decode.vfpv4
```plain
decode(Type_vfpv3_dpi)
{
	if(sz) {
		vd = vd | (D << 4);
		vm = vm | (M << 4);
		vn = vn | (N << 4);
	} else {
		vd = (vd << 1) | D;
		vm = (vm << 1) | M;
		vn = (vn << 1) | N;
	}
}
```

#### /home/maritns3/core/captive-project/gensim/gensim/src/isa/HelperFnDescriptionParser.cpp
examples in /home/maritns3/core/captive-project/gensim/models/armv7/execute.vfpv4

```c
helper uint32 read_cpacr()
{
	// if no cp15, we're in user mode simulation so pretend all coprocs are enabled
	if(!probe_device(15)) return 0x7fffffff;

	return read_coproc_reg(15, 0, 2, 1, 0);
}
```
seems only prototype analzyed

#### /home/maritns3/core/captive-project/gensim/gensim/src/isa/AsmMapDescriptionParser.cpp
- [ ] Work as expected, but we need it ?
```c
	ac_asm_map reg
	{
		"r"[0..15] = [0..15];
		//"a"[1..4] = [0..3];
		//"v"[1..8] = [4..11];
		//"wr" = 7;
		//"sb" = 9;
		"sl" = 10;
		"fp" = 11;
		"ip" = 12;
		"sp" = 13;
		"lr" = 14;
		"pc" = 15;
	}
```

#### /home/maritns3/core/captive-project/gensim/gensim/src/isa/AsmDescriptionParser.cpp
used for analyze:

888:            qdsubt1.set_asm("qdsub<c> %reg, %reg, %reg", rd, rm, rn);

## TODO
- [ ] risc-v is compiled, but not generator found
- [ ] how is src_1.cpp generated ?

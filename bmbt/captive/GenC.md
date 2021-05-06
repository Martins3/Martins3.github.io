Arch
  - ISA
    - BehaviourFiles
    - DecodeFiles
    - ExecuteFiles
    - InstructionFormatMap
    - InstructionDescriptionMap

- [ ] Why we create map for it ?

<!-- vim-markdown-toc GitLab -->

- [source code overview](#source-code-overview)
  - [IR](#ir)
  - [SSA](#ssa)
  - [analysis](#analysis)
  - [io](#io)
  - [statement](#statement)
- [how components works ?](#how-components-works-)
- [SSA](#ssa-1)
  - [how to ssa print ?](#how-to-ssa-print-)
  - [code flow](#code-flow)
    - [Build](#build)
    - [Optimize](#optimize)
    - [Resolve](#resolve)
  - [visitor](#visitor)

<!-- vim-markdown-toc -->

## source code overview

### IR
clean code

### SSA
Oh, it seems dynamic translation never generated :

seems will use the ssa:

DynamicTranslationGenerator::Generate

```c
			if (GetProperty("Debug") == "1") {
```


### analysis

only 500 LOC

### io 
AssemblyReader::Parse used by genc-opt.cpp

- [ ] genc-opt.cpp

### statement
All kind of statement variable with basic line, accept with visitor.


## how components works ?

## SSA

### how to ssa print ?
- [x] why DynamicTranslationGenerator::Generate() never called ?

- DynamicTranslationGenerator::Generate
  - SSAContextPrinter::Print : although we can't generate the code, but 

- [ ] SSA

There are dependency chain of components.
```cpp
DEFINE_COMPONENT(gensim::generator::DynamicTranslationGenerator, translate_dynamic)
COMPONENT_INHERITS(translate_dynamic, translate)
```

- [ ] understand the macros ?

- [ ] different between GetFunction() and GetName() for `class GenerationComponent`

and there is no documents for these components.

- [ ] from SSA to generator ?


ISADescription::BuildSSAContext, will read file which assemble c language:
```
ExecuteFiles : execute.vfpv4
ExecuteFiles : execute.neon
ExecuteFiles : execute.arm
ExecuteFiles : execute.thumb1
ExecuteFiles : execute.thumb_vfp
ExecuteFiles : execute.thumb2
```

### code flow

main:
```c
	for (std::list<isa::ISADescription *>::iterator II = description.ISAs.begin(), IE = description.ISAs.end(); II != IE; ++II) {
		isa::ISADescription *isa = *II;
		bool isasuccess = isa->BuildSSAContext(&description, root_context);

		if(isasuccess) {
			isasuccess &= isa->GetSSAContext().Validate(root_context);
		}

		if (!isasuccess) {
			success = false;
			printf("Errors in arch description for ISA %s.\n", isa->ISAName.c_str());
		} else {
			isa->GetSSAContext().Optimise();
			isa->GetSSAContext().Resolve(root_context);
		}
	}
```

1. Build
2. Optimise
3. Resolve


#### Build
- GenCContext::Parse
  - GenCContext::Parse_File
    - Parse_Constant
    - Parse_Typename
    - Parse_Helper
    - Parse_Behaviour
    - Parse_Execute
      - GenCContext::Parse_Body
        - GenCContext::Parse_Statement
          - case SWITCH
          - ...
          - case EXPR_STMT
            - GenCContext::Parse_Expression
              - case DECL
              - case GENC_ID
  - GenCContext::EmitSSA
    - IRAction::GetSSAForm
      - IRAction::EmitSSA
        - IRBody::EmitSSAForm
          - IRBinaryExpression::EmitSSAForm
          - IRSelectionStatement::EmitSSAForm

Ok, about the prase, so familiar with it.

EmitSSA will create IRAction from SSAFormAction

- IRSelectionStatement::EmitSSAForm
  - `SSABlock *true_block = new SSABlock(bldr);` : we will add SSABlock to SSABuilder
  - `SSABuilder::ChangeBlock` : will create jump statement

#### Optimize
in gensim/src/genC/ssa/passes/AggregatePasses.cpp, define `O3Pass` and `O4Pass`.

```cpp
class O4Pass : public SSAPass
{
public:
	bool Run(SSAFormAction& action) const override
	{
		// O3, then phi analyse, then O3, then phi eliminate, then O3 again
		SSAPassManager manager;
		manager.SetMultirunAll(false);
		manager.SetMultirunEach(true);

		manager.AddPass(SSAPassDB::Get("O3"));
		manager.AddPass(SSAPassDB::Get("PhiAnalysis"));
		manager.AddPass(SSAPassDB::Get("DeadPhiElimination"));
		manager.AddPass(SSAPassDB::Get("O3"));
		manager.AddPass(SSAPassDB::Get("PhiSetElimination"));
		manager.AddPass(SSAPassDB::Get("O3"));

		return manager.Run(action);
	}

};
```

#### Resolve
it's static check.

SSAVariableWriteStatement::Resolve

```cpp
bool SSAVariableWriteStatement::Resolve(DiagnosticContext &ctx)
{
	bool success = true;
	assert(!IsDisposed());

	if(Parent->Parent->Symbols().count(Target()) == 0) {
		throw std::logic_error("");
	}

	if(Expr()->GetType() != Target()->GetType()) {
		throw std::logic_error("A value written to a variable must match the type of the variable");
	}

	// assert(target->IsReference() || Parent->Parent.GetDominatingAllocs(target, this).size() != 0);
	assert(!Target()->IsReference() || Target()->GetReferencee());
	while (Target()->IsReference()) {
		Replace(Target(), Target()->GetReferencee());
	}
	success &= SSAStatement::Resolve(ctx);
	success &= Expr()->Resolve(ctx);
	return success;
}
```

- [ ] pass in /home/maritns3/core/captive-project/gensim/gensim/src/genC/ssa/validation also handle validation

### visitor

use /home/maritns3/core/captive-project/gensim/gensim/src/generators/BlockJIT/JitNodeWalkerFactory.cpp as example

- [ ] class JIT : public gensim::blockjit::BaseBlockJITTranslate create many methoh
  - [ ] how are they genereated ?
  - [ ] how does they work ?
  - [ ] who are they referenced by ?

- EEGenerator::Generate
  - EEGenerator::GenerateHeader ==> BlockJITExecutionEngineGenerator::GenerateHeader
    - JitGenerator::GenerateClass
      - `isa->Formats`
      - `isa->Instructions`
  - BlockJITExecutionEngineGenerator::GenerateSource
    - JitGenerator::GenerateTranslation
      * firstly, create a entry for all instruction : `translate_instruction`
      - JitGenerator::GeneratePredicateFunction
        - JitGenerator::EmitJITFunction
          * action.GetBlocks
          * action.Symbols
          - JitNodeWalkerFactory::Create
          - SSANodeWalker::EmitFixedCode : it will recursively transverse syntax tree, all related code lay in /home/maritns3/core/captive-project/gensim/gensim/src/generators/BlockJIT/JitNodeWalkerFactory.cpp
          - SSANodeWalker::EmitDynamicCode


- [x] how BlockJITExecutionEngineGenerator generated ?

by component ee_blockjit, instead of 
```cpp
DEFINE_COMPONENT(BlockJITExecutionEngineGenerator, ee_blockjit);
```

- [ ] I can't make it work, but it think this is relative to llvm instead of something dynamic : /home/maritns3/core/captive-project/gensim/gensim/src/generators/GenCJIT/DynamicTranslationGenerator.cpp
  - currently, LLVM just doesn't work

- [ ] I don't know how visitor work inside.
  - [ ] Factory
  - [ ] SSAStatementVisitor
  - [ ] HierarchicalSSAStatementVisitor

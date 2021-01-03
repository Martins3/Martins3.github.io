## ArcSim
- main
  - run_simple_simulation
    - System::RunSimulation
      - After a warpper a thread
        - ExecutionEngine::Execute
          - InterpreterExecutionEngine::Execute
            - interpreter_->StepBlock
              - gensim::armv7a::Interpreter::StepBlock(archsim::core::execution::InterpreterExecutionEngineThreadContext *thread)
                - file lied in /home/maritns3/core/captive-project/gensim/build/models/armv7/output-armv7a/ee_interpreter.cpp
                - StepInstruction
                  - StepInstruction_arm
                    - StepInstruction_arm_adc1
          - BasicJITExecutionEngine::Execute
            - BasicJITExecutionEngine::ExecuteLoop
              - BasicJITExecutionEngine::lookupBlock
              - BaseBlockJITTranslate::translate_block : /home/maritns3/core/captive-project/gensim/archsim/src/blockjit 
                  - BaseBlockJITTranslate::build_block
                    - BaseBlockJITTranslate::emit_block
                      - BaseBlockJITTranslate::emit_instruction
                        - `_decode_ctx->DecodeSync` : generated code
                        - BaseBlockJITTranslate::emit_instruction_decoded
                          - JIT::translate_instruction : auto generated code : /home/maritns3/core/captive-project/gensim/build/models/armv7/output-armv7a/ee_blockjit.cpp
                            - translate_arm_adc1 : e.g. So build_block will translate java instruction to
                  - BaseBlockJITTranslate::compile_block
                    - BlockCompiler::compile

/home/maritns3/core/captive-project/gensim/models/CMakeLists.txt
```
		ADD_CUSTOM_COMMAND(
			OUTPUT ${DLL_PATH}
			COMMAND "sh" "-c" "make -C ${CMAKE_CURRENT_BINARY_DIR}/output-${arch-name} -j4"
			DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/output-${arch-name}/Makefile"
			COMMENT "Compiling ${target-name}"
		)
```
- [ ] How make -C is compiled with ArcSim ?

- [ ] variable ARGN

- [ ] How dynamic translation is used by ArcSim
  - there are two way to compile the code interpreter and blockjit

- [ ] **IRInstruction** : instruction is transformed to IRInstruction
  - /home/maritns3/core/captive-project/gensim/archsim/inc/blockjit/IRInstruction.h
  1. If it's translated to IRInstruction, what's difference with Qemu ?

user mode and kernel mode has a different engine.

## build block
## compile block


## Engine & Model
- [ ] engine :  Interpreter or JIT, is determined by the option ?

```c
bool System::RunSimulation()
{
	if (!emulation_model->PrepareBoot(*this)) return false;
	GetECM().Start();
	GetECM().Join();

	return true;
}
```

- main
  - run_simple_simulation
    - System::Initialise() : choose Emulunation Model : armv7
      - UserEmulationModel::Initialise
        - `execution_engine_` =  ExecutionEngineFactory::Get : This is engine that we get.
        - `GetSystem().GetECM().AddEngine(execution_engine_)`
        - UserEmulationModel::StartThread
          - ExecutionEngine::AttachThread

```c
void ExecutionEngineThreadContext::Execute()
{
	engine_->Execute(this);
}

void UserEmulationModel::StartThread(archsim::core::thread::ThreadInstance* thread)
{
	execution_engine_->AttachThread(thread);
}
```

- [ ] How they works for a same instruction
- [ ] So we generate code for interpreter and JIT separately ?

/home/maritns3/core/captive-project/gensim/archsim/src/core/execution/InterpreterExecutionEngine.cpp
```c
static archsim::core::execution::ExecutionEngineFactoryRegistration registration("Interpreter", 10, archsim::core::execution::InterpreterExecutionEngine::Factory);
```
/home/maritns3/core/captive-project/gensim/archsim/src/core/execution/BlockJITExecutionEngine.cpp
```c
static archsim::core::execution::ExecutionEngineFactoryRegistration registration("BlockJIT", 100, archsim::core::execution::BlockJITExecutionEngine::Factory);
```

Execution engine will be choosed with priorities:
```cpp
void ExecutionEngineFactory::Register(const std::string& name, int priority, EEFactory factory)
{
	Entry entry;
	entry.Name = name;
	entry.Factory = factory;
	entry.Priority = priority;
	factories_.insert({priority, entry});
}
```

Maybe, read the document why they create two engine.

## BlockCompiler::compile
- BaseBlockJITTranslate::compile_block
  - BlockCompiler::compile


## Lowering
- BaseBlockJITTranslate::compile_block
  - captive::arch::jit::lowering::NativeLowering
    - MCLoweringContext::Lower
      - LoweringContext::Lower
        - X86LoweringContext::LowerHeader
        - LoweringContext::LowerBody
          - LoweringContext::LowerBlock
            - LoweringContext::LowerInstruction
              - _lowerers[insn->type]->Lower(insn); : call x86 lowering
                - Encoder().add()
                - Encoder().move()

entry : archsim/src/blockjit/block-compiler/lowering/x86/NativeLowering.cpp

- IRInstruction
- InstructionLowerer


Some trick to create class and it's singleton


1. create class
```
#define LowerType(X) class Lower ## X : public X86InstructionLowerer { public: Lower ## X() : X86InstructionLowerer() {} bool Lower(const captive::shared::IRInstruction *&insn) override; };
#define LowerTypeTS(X) class Lower ## X;
#include "blockjit/block-compiler/lowering/x86/LowerTypes.h"
#undef LowerType
#undef LowerTypeTS
```

2. create singleton
```
#define LowerType(X) Lower ## X Singleton_Lower ## X;
#define LowerTypeTS(X) Lower ## X Singleton_Lower ## X;
#include "blockjit/block-compiler/lowering/x86/LowerTypes.h"
#undef LowerType
```

So, how Lower is reimplemented ?

- [ ] IRBuilder: so many macros ?

- TranslationContext 
  - `shared::IRBlockId _current_block_id;`
  - `shared::IRInstruction *_ir_insns;`

TranslationContext contains IRInstruction of current IRBloc.


IRBuilder provide all kinds of function to add IRInstruction to TranslationContext:
```cpp
#define INSN0(x, y) void x() { auto insn = new(ctx_->get_next_instruction_ptr()) IRInstruction(IRInstruction::y);  insn->ir_block = GetBlock(); }
```

## LLVM block JIT
related dir src/translate, it really resembling how blockjit works.

## Wow, the last question, how abi, arch works for them ?
- [ ] we even try to simulate syscall

- abi is used for emulate device for system, and syscall for userspace

## meminterface
```cpp
MemoryInterface& ThreadInstance::GetMemoryInterface(uint32_t interface_name)
{
	return *memory_interfaces_.at(interface_name);
}

    // called by src_1.cpp
		MemoryResult Read32(Address address, uint32_t &data)
		{
			return device_->Read32(address, data);
		}

MemoryResult CachedLegacyMemoryInterface::Read32(Address address, uint32_t& data)
MemoryResult LegacyFetchMemoryInterface::Read32(Address address, uint32_t& data)
MemoryResult LegacyMemoryInterface::Read32(Address address, uint32_t& data)
```
and these three memory interface will call 6 different memory device

And only `ContiguousMemoryModel::Write` is called.

- [x] How ContiguousMemoryModel is registered ?

in `UserEmulationModel::CreateThread`:
```cpp
	for(auto i : thread->GetMemoryInterfaces()) {
		i->Connect(*new archsim::CachedLegacyMemoryInterface(idx, GetMemoryModel(), thread));
		i->ConnectTranslationProvider(*new archsim::IdentityTranslationProvider());
		i->SetMonitor(monitor_);
		idx++;
	}
```

```cpp
bool EmulationModel::Initialise(System& system, uarch::uArch& uarch)
{
	this->system = &system;
	this->uarch = &uarch;

	if (!GetComponentInstance(archsim::options::MemoryModel, memory_model)) {
		LC_ERROR(LogSystem) << "Unable to create memory model '" << archsim::options::MemoryModel.GetValue() << "'";
		return false;
	}

	if (!memory_model->Initialise()) {
		LC_ERROR(LogSystem) << "Unable to initialise memory model '" << archsim::options::MemoryModel.GetValue() << "'";
		return false;
	}

	return true;
}

// /home/maritns3/core/captive-project/gensim/archsim/src/abi/memory/ContiguousMemoryModel.cpp
RegisterComponent(MemoryModel, ContiguousMemoryModel, "contiguous", "");
```

- [ ] What's relation with ContiguousMemoryModel::Write and ContiguousMemoryModel::Write32 ?
  - [ ] Whatever how ContiguousMemoryModel::Write or ContiguousMemoryModel::Write32 are called, they are called before BasicJITExecutionEngine::Execute


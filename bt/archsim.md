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
          - BasicJITExecutionEngine::Execute
            - BasicJITExecutionEngine::ExecuteLoop
              - BasicJITExecutionEngine::lookupBlock
              - BaseBlockJITTranslate::translate_block : /home/maritns3/core/captive-project/gensim/archsim/src/blockjit 
                  - BaseBlockJITTranslate::build_block
                  - BaseBlockJITTranslate::compile_block

- [ ] So gensim generated the code and ArcSim compile with it ?
- [ ] blockjit : seems what It read

- [ ] how to build the dependency, 


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

- [ ] ARGN


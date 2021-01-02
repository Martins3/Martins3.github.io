Arch
  - ISA
    - BehaviourFiles
    - DecodeFiles
    - ExecuteFiles
    - InstructionFormatMap
    - InstructionDescriptionMap

- [ ] Why we create map for it ?



## IR
clean code

## SSA
Oh, it seems dynamic translation never generated :

seems will use the ssa:

DynamicTranslationGenerator::Generate

```c
			if (GetProperty("Debug") == "1") {
```


### analysis

only 500 LOC

## io 
AssemblyReader::Parse used by genc-opt.cpp

- [ ] genc-opt.cpp

## statement
All kind of statement variable with basic line, accept with visitor.

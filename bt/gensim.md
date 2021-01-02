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

```
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

- [ ] 


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

```
COMMAND ${gensim-binary} -a ${CMAKE_CURRENT_SOURCE_DIR}/${arch-file} ${gensim-options} -t ${CMAKE_CURRENT_BINARY_DIR}/output-${arch-name}/
```

```
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


## TODO
- [ ] risc-v is compiled, but not generator found

- [ ] how is src_1.cpp generated ?

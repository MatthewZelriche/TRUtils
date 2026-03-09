target_compile_options(
   ${PROJECT_NAME}
   INTERFACE
   $<$<CXX_COMPILER_ID:MSVC>:
      /W4 # Enable most warnings
      /w44242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
      /w44254 # 'operator': conversion from 'type1' to 'type2', possible loss of data
      /w44263 # member function does not override any base class virtual member function
      /w44265 # 'class': class has virtual functions, but destructor is not virtual
      /w44287 # 'operator': unsigned/negative constant mismatch
      /w44388 # signed/unsigned mismatch
      /w45038 # data member 'member1' will be initialized after data member 'member2'
      /w44062 # Missing case in Switch Statement with no default
      /permissive-
   >

   $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
      -Wall
      -Wextra
      # TODO: Others
   >
)

set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_WARNING_AS_ERROR ON)
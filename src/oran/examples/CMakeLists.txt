build_lib_example(
  NAME oran-random-walk-example
  SOURCE_FILES oran-random-walk-example.cc
  LIBRARIES_TO_LINK
   ${liboran}
)

 build_lib_example(
   NAME oran-nr-example-rl-dqn
   SOURCE_FILES oran-nr-example-rl-dqn.cc
   LIBRARIES_TO_LINK
    ${liboran}
 )

build_lib_example(
  NAME oran-nr-example-rl-qlearn
  SOURCE_FILES oran-nr-example-rl-qlearn.cc
  LIBRARIES_TO_LINK
   ${liboran}
)


# CHECK WE HAVE ALL THE VARIABLES WE NEED

message("test_include_dirs     ${TEST_INCLUDE_DIRS}")
message("test_link_libs        ${TEST_LINK_LIBS}")

set(TARGET test_demo_parser)
add_executable(${TARGET}
		test_demo_parser.c
		)

target_include_directories(${TARGET}
		PUBLIC
		${TEST_INCLUDE_dirS})

target_link_libraries(${TARGET}
		PUBLIC ${TEST_LINK_LIBS})

add_test(${TARGET} ${TARGET})



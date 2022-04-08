include_directories(${CMAKE_SOURCE_DIR}/src)

find_package(SQLITE3 REQUIRED)

include_directories( ${SQLITE3_INCLUDE_DIRS} )

add_definitions(-DSOURCE_DIRECTORY_=${CMAKE_SOURCE_DIR}/src)

add_library( mator SHARED
	mator.cpp
	ip.cpp
	utils.cpp
	relay.cpp
	db_connection.cpp
	consensus.cpp
	asmap.cpp
	types/work_manager.cpp
	relationship_manager.cpp
	path_selection_standard.cpp
	tor_like.cpp
	ps_tor.cpp
	ps_uniform.cpp
	ps_selektor.cpp
	ps_distributor.cpp
	ps_lastor.cpp
	pythonbinding.cpp
	scenario.cpp
	costmap.cpp
	pcf.cpp
	pcf_parser.cpp
	generic_worst_case_anonymity.cpp
	generic_precise_anonymity.cpp
)

add_library( simple_json STATIC
	SimpleJSON/JSON.cpp
	SimpleJSON/JSONValue.cpp
)

target_link_libraries ( mator
	simple_json
	${SQLITE3_LIBRARIES}
)

add_executable(runtest maintest.cpp)
target_link_libraries (runtest LINK_PUBLIC mator)


set_property(TARGET mator PROPERTY CXX_STANDARD 11)

enable_testing()

find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Sourcing engine unit tests
add_executable(test_sourcing_engine
    test_sourcing_engine.cpp
    ${CMAKE_SOURCE_DIR}/src/models/Session.cpp
    ${CMAKE_SOURCE_DIR}/src/data/LocalDataProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/engine/SourcingEngine.cpp
)

target_include_directories(test_sourcing_engine PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${Wt_INCLUDE_DIRS}
)

target_link_libraries(test_sourcing_engine
    Wt::Wt
    Wt::Dbo
    Wt::DboSqlite3
    Boost::system
    Boost::filesystem
    Boost::thread
    Boost::unit_test_framework
)

target_compile_definitions(test_sourcing_engine PRIVATE BOOST_TEST_DYN_LINK)

add_test(NAME SourcingEngineTests COMMAND test_sourcing_engine)

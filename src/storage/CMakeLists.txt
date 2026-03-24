add_library(storage
    TableManager.cpp
    IndexManager.cpp
    BPlusTree.cpp
    FileIO.cpp
)

target_include_directories(storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

target_link_libraries(storage PRIVATE common Qt${QT_VERSION_MAJOR}::Core)

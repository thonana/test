set(APPNAME demo_basic)

project(${APPNAME})

add_executable(${APPNAME} WIN32
	app.cc
)

target_link_libraries(${APPNAME}
PUBLIC 
    general     base
)
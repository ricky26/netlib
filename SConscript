Import('*')

env.Append(CPPPATH=Dir('include'))
env.Append(CPPFLAGS='-std=gnu++0x')

netlib_src = [
	"src/base64_linux.cpp",
	"src/bitstream.cpp",
	"src/crypto.cpp",
	"src/file_linux.cpp",
	"src/http.cpp",
    "src/i18n_linux.cpp",
    "src/internal.cpp",
	"src/irc.cpp",
	"src/json.cpp",
	"src/md5_linux.cpp",
	"src/netlib_linux.cpp",
	"src/pipe_linux.cpp",
	"src/ref_counted.cpp",
    "src/resource_linux.cpp",
	"src/sha1_linux.cpp",
	"src/shared_memory_linux.cpp",
	"src/socket_linux.cpp",
	"src/string.cpp",
	"src/telnet.cpp",
	"src/terminal.cpp",
	"src/thread_linux.cpp",
	"src/uthread_linux.cpp",
	"src/websocket.cpp",
]

netlib = env.SharedLibrary('netlib', netlib_src, LIBS=['ssl', 'pthread', 'rt'])
Export('netlib')

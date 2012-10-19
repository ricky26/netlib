import platform

vars = Variables(['variables.cache', 'custom.py'])
vars.Add(BoolVariable('DEBUG', 'Set whether the program will be compiled in debug mode.', False))
vars.Add('ARCH', 'Set the target architecture', platform.machine())

env = Environment(variables=vars)
Export("env")

if env['ARCH'] == 'x86_64':
   env.Append(CPPFLAGS=[
		'-m64', '-DNETLIB_X64'
		], ASFLAGS=['--64'])
elif env['ARCH'] == 'i386':
   env.Append(CPPFLAGS=[
		'-m32', '-DNETLIB_X86'
		], ASFLAGS=['--32'])

if env['DEBUG']:
   env.Append(CPPFLAGS=[
		'-DDEBUG', '-g'
		], ASFLAGS=['-g'])

env.SConscript("SConscript")

vars.Save('variables.cache', env)

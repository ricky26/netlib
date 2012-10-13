vars = Variables(['variables.cache', 'custom.py'])
vars.Add(BoolVariable('DEBUG', 'Set whether the program will be compiled in debug mode.', False))

env = Environment(variables=vars)
Export("env")

if env['DEBUG']:
   env.Append(CPPFLAGS=[
		'-DDEBUG', '-g'
		])

env.SConscript("SConscript")

vars.Save('variables.cache', env)

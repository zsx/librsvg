# vim: ft=python expandtab
import os
from site_init import GBuilder, GInitialize

opts = Variables()
opts.Add(PathVariable('PREFIX', 'Installation prefix', os.path.expanduser('~/FOSS'), PathVariable.PathIsDirCreate))
opts.Add(BoolVariable('DEBUG', 'Build with Debugging information', 0))
opts.Add(PathVariable('PERL', 'Path to the executable perl', r'C:\Perl\bin\perl.exe', PathVariable.PathIsFile))

env = Environment(variables = opts,
                  ENV=os.environ, tools = ['default', GBuilder])
GInitialize(env)
env['PDB'] = 'librsvg.pdb'
env['ESCAPED_PREFIX'] = env['PREFIX'].replace('\\', '\\\\')

LIBRSVG_MAJOR_VERSION=2
LIBRSVG_MINOR_VERSION=26
LIBRSVG_MICRO_VERSION=0
LIBRSVG_VERSION="%d.%d.%d" % (LIBRSVG_MAJOR_VERSION, LIBRSVG_MINOR_VERSION, LIBRSVG_MICRO_VERSION)
env['LIBRSVG_VERSION'] = LIBRSVG_VERSION
env['DOT_IN_SUBS'] = {'@PACKAGE_VERSION@': LIBRSVG_VERSION,
		      '@VERSION@': LIBRSVG_VERSION,
                      '@LIBRSVG_MAJOR_VERSION@': str(LIBRSVG_MAJOR_VERSION),
                      '@LIBRSVG_MINOR_VERSION@': str(LIBRSVG_MINOR_VERSION),
                      '@LIBRSVG_MICRO_VERSION@': str(LIBRSVG_MICRO_VERSION),
                      '@LIBRSVG_VERSION@': LIBRSVG_VERSION,
                      '@prefix@': env['PREFIX'],
                      '@exec_prefix@': '${prefix}/bin',
                      '@libdir@': '${prefix}/lib',
                      '@includedir@': '${prefix}/include',
                      '@LIBRSVG_HAVE_SVGZ@': '0',
                      '@LIBRSVG_HAVE_CSS@': '0'
                      }
env.DotIn('librsvg-2.0.pc', 'librsvg-2.0.pc.in')
env.Alias('install', env.Install('$PREFIX/lib/pkgconfig', 'librsvg-2.0.pc'))

env.DotIn('config.h', 'config.h.win32.in')
env.DotIn('librsvg-features.h', 'librsvg-features.h.in')

subdirs = ['gdk-pixbuf-loader/SConscript']
#broken           'gtk-engine/SConscript']
if ARGUMENTS.get('build_test', 0):
    subdirs += ['tests/SConscript']

#env.ParseConfig('pkg-config glib-2.0 --cflags --libs')
#env.ParseConfig('pkg-config gthread-2.0 --cflags --libs')
#env.ParseConfig('pkg-config cairo --cflags --libs')
#env.ParseConfig('pkg-config cairo-png --cflags --libs')
#env.ParseConfig('pkg-config pangocairo --cflags --libs')
env.ParseConfig('pkg-config libxml-2.0 --cflags --libs')
#env.ParseConfig('pkg-config gio-2.0 --cflags --libs')
env.ParseConfig('pkg-config gtk+-2.0 --cflags --libs')
#env.ParseConfig('pkg-config gdk-pixbuf-2.0 --cflags --libs')
env.ParseConfig('pkg-config freetype2 --cflags --libs')

SConscript(subdirs, exports=['env'])

headers = Split("\
	rsvg.h	\
	rsvg-cairo.h")

enum_sources = Split("\
	librsvg-enum-types.h	\
	librsvg-enum-types.c")

librsvg_SOURCES = Split("\
	rsvg-affine.c		\
	librsvg-features.c 	\
	rsvg-bpath-util.c 	\
	rsvg-css.c 		\
	rsvg-defs.c 		\
	rsvg-image.c		\
	rsvg-paint-server.c 	\
	rsvg-path.c 		\
	rsvg-base-file-util.c 	\
	rsvg-filter.c		\
	rsvg-marker.c		\
	rsvg-mask.c		\
	rsvg-shapes.c		\
	rsvg-structure.c	\
	rsvg-styles.c		\
	rsvg-text.c		\
	rsvg-cond.c		\
	rsvg-base.c		\
	librsvg-enum-types.c	\
	rsvg-cairo-draw.c	\
	rsvg-cairo-render.c	\
	rsvg-cairo-clip.c	\
	rsvg.c			\
	rsvg-gobject.c          \
	rsvg-file-util.c")

#
env_mkenums_h = env.Clone()
env_mkenums_h['GLIB_MKENUMS_ARGV'] = (
                        ("fhead", r'"#ifndef __LIBRSVG_ENUM_TYPES_H__\n#define __LIBRSVG_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n"'),
			("fprod", r'"/* enumerations from \"@filename@\" */\n"'),
			("vhead", r'"GType @enum_name@_get_type (void);\n#define RSVG_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n"'),
			("ftail", r'"G_END_DECLS\n\n#endif /* __LIBRSVG_ENUM_TYPES_H__ */"')
                        )
env_mkenums_h.Depends(env_mkenums_h.MkenumsGenerator('librsvg-enum-types.h', headers), 'SConstruct')

env_mkenums_c = env.Clone()
env_mkenums_c['GLIB_MKENUMS_ARGV'] = (
                        ("fhead", r'"#include \"librsvg-enum-types.h\"\n#include \"rsvg.h\""'),
			("fprod", r'"\n/* enumerations from \"@filename@\" */"'),
			("vhead", r'"GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {"'),
			("vprod", r'"      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" },"'),
			("vtail", r'"      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n"')
                        )
env_mkenums_h.Depends(env_mkenums_c.MkenumsGenerator('librsvg-enum-types.c', headers), 'SConstruct')

rsvg_dll=env.SharedLibrary(['librsvg-2' + env['LIB_SUFFIX'] + env['SHLIBSUFFIX'],
                               'librsvg-2.lib'],
                               librsvg_SOURCES + ['librsvg.def'])
env.AddPostAction(rsvg_dll, 'mt.exe -nologo -manifest ${TARGET}.manifest -outputresource:$TARGET;2')

Alias('install', env.Install('$PREFIX/include/librsvg-2/librsvg', headers))
Alias('install', env.Install('$PREFIX/bin', 'librsvg-2' + env['LIB_SUFFIX'] + '.dll'))
Alias('install', env.Install('$PREFIX/lib', 'librsvg-2.lib'))
Alias('install', env.InstallAs('$PREFIX/lib/rsvg-2.lib', 'librsvg-2.lib'))
if env['DEBUG'] == 1:
    Alias('install', env.Install('$PREFIX/pdb', env['PDB']))

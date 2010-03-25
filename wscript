#! /usr/bin/env python

APPNAME = 'vteplugin'

VERSION = '0.1'

top = '.'
blddir = 'build'

def configure(ctx):
    import Options
    if Options.options.debug:
        ctx.env['CFLAGS'] = ['-g']
        ctx.env['CPPFLAGS'] = ['-DDEBUG']
    else:
        ctx.env['CFLAGS'] = ['-O2', '-Wall']

    ctx.check_tool('gcc')
    ctx.check_cfg(package='gtk+-2.0', args='--cflags --libs', uselib_store="gtk", mandatory=True)
    ctx.check_cfg(package='gdk-2.0', args='--cflags --libs', uselib_store="gdk", mandatory=True)
    ctx.check_cfg(package='vte', args='--cflags --libs', uselib_store="vte", mandatory=True)

def set_options (opt):
    opt.add_option ('--debug', action='store_true', default=False, help='Debug mode', dest='debug')

def build(ctx):
    import os.path
    ctx.env.shlib_PATTERN = "%s.so"
    obj = ctx.new_task_gen(features='cc cshlib', 
                           target=APPNAME,
                           install_path="/usr/lib/mozilla/plugins",
                           uselib="vte gdk gtk")
    obj.find_sources_in_dirs(top)

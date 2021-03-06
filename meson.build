# Copyright 2018 Endless Mobile, Inc.

project('dmodel', 'c', version: '0.1.0', license: 'LGPL2.1+',
    meson_version: '>= 0.42.0',
    default_options: ['c_std=c99', 'warning_level=3'])

gnome = import('gnome')
pkg = import('pkgconfig')

version_components = meson.project_version().split('.')
api_version = version_components[0]
api_name = '@0@-@1@'.format(meson.project_name(), api_version)
libtool_version = '0.0.0'
namespace_name = 'DModel'

# Build options

jasmine_report_argument = ''
junit_reports_dir = get_option('jasmine_junit_reports_dir')
if junit_reports_dir != ''
    jasmine_report_argument = '--junit=@0@/${log/.log/.js.xml}'.format(junit_reports_dir)
endif

# Dependencies

libendless_req = '>= 0.5'
shard_req = '>= 0.1.2'

gio = dependency('gio-2.0')
glib = dependency('glib-2.0')
gmodule = dependency('gmodule-2.0')
gobject = dependency('gobject-2.0')
json_glib = dependency('json-glib-1.0')
libarchive = dependency('libarchive')
libendless = dependency('endless-0', version: libendless_req)
shard = dependency('eos-shard-0', version: shard_req)
soup = dependency('libsoup-2.4')
xapian_glib = dependency('xapian-glib-2.0')
zim_glib = dependency('zim-glib-3.0')

giomoduledir = gio.get_pkgconfig_variable('giomoduledir')

main_library_dependencies = [
    gio,
    glib,
    gobject,
    json_glib,
    libarchive,
    libendless,
    shard,
    soup,
    xapian_glib,
    zim_glib,
]

subdir('dmodel')
subdir('eknvfs')

# Dependency object for gtk-doc and tests to use
main_library_dependency = declare_dependency(dependencies: [glib, gobject],
    include_directories: include, link_with: main_library)

# Data files

install_data('overrides/@0@.js'.format(namespace_name),
    install_dir: join_paths(get_option('datadir'), 'gjs-1.0', 'overrides'))

# Generated Files

config = configuration_data()
config.set_quoted('DM_VERSION', meson.project_version())
configure_file(configuration: config, output: 'config.h')

requires = ['glib-2.0', 'gio-2.0', 'gobject-2.0', 'json-glib-1.0', 'libsoup-2.4', 'libarchive']
requires_private = [
    'endless-0 @0@'.format(libendless_req),
    'eos-shard-0 @0@'.format(shard_req),
    'xapian-glib-2.0',
    'zim-glib-3.0',
]
pkg.generate(filebase: api_name, libraries: [main_library],
    description: 'Software development kit for applications for the developing world',
    name: meson.project_name(), subdirs: api_name, requires: requires,
    requires_private: requires_private,
    url: 'http://endlessm.github.io/eos-knowledge-lib',
    version: meson.project_version())

subdir('tests')

if get_option('documentation')
    subdir('docs')
endif

# Summary

message('\n'.join([
    '@0@ @1@ (@2@)'.format(meson.project_name(), meson.project_version(),
        api_name),
    '-------------------',
    'Options:',
    '     Documentation: @0@'.format(get_option('documentation')),
    '  Test reports dir: @0@'.format(get_option('jasmine_junit_reports_dir')),
    '',
    'Directories:',
    '    Install prefix: @0@'.format(get_option('prefix')),
    '   GIO Modules dir: @0@'.format(giomoduledir),
    ''
]))

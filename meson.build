project(
  'Dear ImGui',
  'cpp',
  meson_version : '>= 0.46.0',
  default_options : ['default_library=both'],
  license : 'MIT',
  version : '1.75',
)

dearimgui_inc = include_directories('.')
dearimgui_src = files(
  'imgui.cpp',
  'imgui_draw.cpp',
  'imgui_widgets.cpp',
)

dearimgui_lib = library(
  'dearimgui',
  dearimgui_src,
  include_directories : dearimgui_inc,
  install : true,
  version : meson.project_version(),
  soversion : '0',
)

install_headers('imgui.h')

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  dearimgui_lib,
  name : 'dearimgui',
  description : 'Immediate mode graphical user interface',
  version : meson.project_version(),
)

dearimgui_dep = declare_dependency(
  link_with : dearimgui_lib,
  include_directories : dearimgui_inc,
)

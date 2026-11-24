project('lab1', 'c', default_options : ['c_std=c11', 'warning_level=2'])

cc = meson.get_compiler('c')

common_sources = files('main.c', 'runner.c')

# Правильный способ определить ОС в Meson:
if host_machine.system() == 'windows'
  platform_sources = files('platform_win.c')
  platform_deps = []
else
  platform_sources = files('platform_posix.c')
  # В Linux для shm_open/sem_open нужна librt, для pthread - libpthread
  platform_deps = [
    cc.find_library('rt', required : true),
    cc.find_library('pthread', required : true)
  ]
endif

executable('lab1',
  common_sources + platform_sources,
  dependencies : platform_deps,
  install : true)
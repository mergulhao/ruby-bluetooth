# Loads mkmf which is used to make makefiles for Ruby extensions
require 'mkmf'

# Give it a name
extension_name = 'ruby_bluetooth'

dir_config(extension_name)

case RUBY_PLATFORM
  when /linux/
    if have_library('bluetooth')
      create_makefile(extension_name, 'bluetooth_linux')
    end
  when /darwin/
      create_makefile(extension_name, 'bluetooth_macosx')
  when /(win32|mingw32)/
      create_makefile(extension_name, 'bluetooth_win32')
end

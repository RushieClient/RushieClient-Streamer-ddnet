find_package(PkgConfig QUIET)
pkg_check_modules(GIO QUIET gio-2.0)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gio DEFAULT_MSG GIO_LIBRARIES GIO_INCLUDE_DIRS)

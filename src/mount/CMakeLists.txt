add_executable("mount.sfs"
    "main.cpp"
)
target_link_libraries("mount.sfs" PRIVATE "argparse")

find_package(PkgConfig)
pkg_check_modules(fuse REQUIRED fuse)
target_link_libraries("mount.sfs" PRIVATE ${fuse_LIBRARIES})
target_include_directories("mount.sfs" PRIVATE ${fuse_INCLUDE_DIRS})
target_compile_options("mount.sfs" PRIVATE ${fuse_CFLAGS})

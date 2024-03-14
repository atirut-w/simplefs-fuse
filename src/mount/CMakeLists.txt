add_executable("mount.sfs"
    "main.cpp"
)
target_link_libraries("mount.sfs" PRIVATE "argparse")

#include <argparse/argparse.hpp>
#include <memory>
#include <cstdio>
#include <simplefs.hpp>
#include <cstring>
#include <cstdint>
#include <vector>
#include <fuse.h>

FILE *image_file;
Superblock superblock;
std::vector<uint8_t> blockmap;
NamelistEntry root;

void seek(int block, int offset = 0)
{
    fseek(image_file, block * superblock.block_size + offset, SEEK_SET);
}

NamelistEntry get_namelist_entry(int id)
{
    NamelistEntry entry;
    
    int block = (id * 128) / superblock.block_size;
    int offset = (id * 128) % superblock.block_size;
    seek(2 + block, offset);

    fread(&entry, sizeof(NamelistEntry), 1, image_file);
    return entry;
}

std::shared_ptr<const argparse::ArgumentParser> parse_arguments(int argc, char *argv[])
{
    auto parser = std::make_shared<argparse::ArgumentParser>("mount.sfs");

    parser->add_argument("image")
        .help("The SimpleFS image to mount.");
    
    parser->add_argument("mountpoint")
        .help("The mount point to mount the image to.");

    try
    {
        parser->parse_args(argc, argv);
    }
    catch (const std::exception &err)
    {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return nullptr;
    }
    return parser;
}

int main(int argc, char *argv[])
{
    auto args = parse_arguments(argc, argv);
    if (!args)
        return 1;
    
    image_file = fopen(args->get<std::string>("image").c_str(), "rb+");
    if (image_file == nullptr)
    {
        std::cerr << "Failed to open " << args->get<std::string>("image") << std::endl;
        return 1;
    }

    fread(&superblock, sizeof(Superblock), 1, image_file);
    if (memcmp(superblock.signature, "\x1bSFS", 4) != 0)
    {
        std::cerr << "Not a SimpleFS image" << std::endl;
        return 1;
    }
    seek(1);
    blockmap.resize(superblock.block_size);
    fread(blockmap.data(), 1, superblock.block_size, image_file);
    seek(2);
    fread(&root, sizeof(NamelistEntry), 1, image_file);

    fuse_operations operations = {

    };

    fuse_mount(args->get<std::string>("mountpoint").c_str(), nullptr);
    fuse *f = fuse_new(fileno(image_file), nullptr, &operations);

    fuse_loop(f);

    fuse_unmount(args->get<std::string>("mountpoint").c_str());
    fuse_destroy(f);
    
    return 0;
}

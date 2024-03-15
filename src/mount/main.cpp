#include <argparse/argparse.hpp>
#include <memory>
#include <fstream>
#include <simplefs.hpp>
#include <cstring>
#include <cstdint>
#include <vector>

std::shared_ptr<std::fstream> image_file;
Superblock superblock;
std::vector<uint8_t> blockmap;
NamelistEntry root;

void seek(int block, int offset = 0)
{
    image_file->seekg((block * superblock.block_size) + offset, std::ios::beg);
    image_file->seekp((block * superblock.block_size) + offset, std::ios::beg);
}

NamelistEntry get_namelist_entry(int id)
{
    NamelistEntry entry;
    
    int block = (id * 128) / superblock.block_size;
    int offset = (id * 128) % superblock.block_size;
    seek(2 + block, offset);

    image_file->read(reinterpret_cast<char *>(&entry), sizeof(NamelistEntry));
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
    
    image_file = std::make_shared<std::fstream>(args->get<std::string>("image"), std::ios::in | std::ios::out | std::ios::binary);
    if (!image_file->is_open())
    {
        std::cerr << "Failed to open " << args->get<std::string>("image") << std::endl;
        return 1;
    }

    image_file->read((char *)&superblock, sizeof(Superblock));
    if (memcmp(superblock.signature, "\x1bSFS", 4) != 0)
    {
        std::cerr << "Not a SimpleFS image" << std::endl;
        return 1;
    }
    seek(1);
    blockmap.resize(superblock.block_size);
    image_file->read((char *)blockmap.data(), superblock.block_size);
    seek(2);
    image_file->read((char *)&root, sizeof(NamelistEntry));
    
    return 0;
}

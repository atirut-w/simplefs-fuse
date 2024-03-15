#define FUSE_USE_VERSION 31

#include <argparse/argparse.hpp>
#include <memory>
#include <cstdio>
#include <simplefs.hpp>
#include <cstring>
#include <cstdint>
#include <vector>
#include <fuse.h>
#include <filesystem>
#include <sys/stat.h>
#include <algorithm>

FILE *image_file;
Superblock superblock;
std::vector<uint8_t> blockmap;
std::vector<NamelistEntry> namelist;

void seek_block(int block)
{
    fseek(image_file, block * superblock.block_size, SEEK_SET);
}

NamelistEntry get_namelist_path(std::filesystem::path path)
{
    NamelistEntry current_dir = namelist[0];
    if (path == "/")
        return current_dir;

    for (auto part : path)
    {
        NamelistEntry current;
        auto current_id = current_dir.datablock;

        do
        {
            current = namelist[current_id++];
            if ((char *)current.fname == part)
            {
                current_dir = current;
                break;
            }
        } while (current.next != 0);

        // if ((char *)current.fname == part)
        // {
        //     current_dir = current;
        // }
        // else
        // {
        //     throw std::runtime_error("File not found");
        // }
    }

    return current_dir;
}

static int do_getattr(const char *path, struct stat *st)
{
    try
    {
        auto entry = get_namelist_path(path);

        st->st_uid = entry.uid;
        st->st_gid = entry.gid;
        st->st_mode = entry.flags;
        st->st_size = entry.size;
        st->st_blksize = superblock.block_size;

        if (entry.flags & S_IFDIR)
            st->st_nlink = 2;

        return 0;
    }
    catch (const std::exception &err)
    {
        return -1;
    }
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    try
    {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        auto dir = get_namelist_path(path);
        NamelistEntry current;
        auto current_id = dir.datablock;
        do
        {
            current = namelist[current_id++];
            filler(buf, (char *)current.fname, nullptr, 0);
        } while (current.next != 0);

        return 0;
    }
    catch (const std::exception &err)
    {
        return -1;
    }
}

static int do_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    try
    {
        auto entry = get_namelist_path(path);
        seek_block(entry.datablock);
        int block_data_size = superblock.block_size - 3; // A datablock ends with a pointer to the next block

        int num_skips = offset / superblock.block_size;
        for (int i = 0; i < num_skips; i++)
        {
            uint16_t next_block;
            fseek(image_file, block_data_size, SEEK_CUR);
            fread(&next_block, 1, 2, image_file);
            seek_block(next_block);
        }

        int num_blocks = size / block_data_size;
        int remaining = size;
        for (int i = 0; i < num_blocks; i++)
        {
            if (i == 0 && offset % block_data_size > 0)
            {
                fseek(image_file, offset % block_data_size, SEEK_CUR);
                buf += fread(buf, 1, std::min<int>(remaining, block_data_size - offset % block_data_size), image_file);
                remaining -= std::min<int>(remaining, block_data_size - offset % block_data_size);
            }
            else
            {
                buf += fread(buf, 1, std::min<int>(remaining, block_data_size), image_file);
                remaining -= std::min<int>(remaining, block_data_size);
            }
            uint16_t next_block = 0;
            fread(&next_block, 1, 2, image_file);
            seek_block(next_block);
        }

        return entry.size;
    }
    catch (const std::exception &err)
    {
        return -1;
    }
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

static fuse_operations operations = {
    .getattr = do_getattr,
    .read = do_read,
    .readdir = do_readdir,
};

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
    seek_block(1);
    blockmap.resize(superblock.block_size);
    fread(blockmap.data(), 1, superblock.block_size, image_file);
    namelist.resize((superblock.block_size / sizeof(NamelistEntry)) * superblock.namelist_blocks);
    fread(namelist.data(), 1, superblock.block_size * superblock.namelist_blocks, image_file);

    fuse_args f_args = FUSE_ARGS_INIT(0, nullptr);
    fuse_chan *channel = fuse_mount(args->get<std::string>("mountpoint").c_str(), &f_args);
    if (channel == nullptr)
    {
        std::cerr << "Failed to mount " << args->get<std::string>("mountpoint") << std::endl;
        return 1;
    }

    fuse *f = fuse_new(channel, &f_args, &operations, sizeof(operations), nullptr);
    fuse_loop(f);

    fuse_unmount(args->get<std::string>("mountpoint").c_str(), channel);
    fuse_destroy(f);
    return 0;
}

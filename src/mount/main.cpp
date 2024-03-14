#include <argparse/argparse.hpp>
#include <memory>

std::shared_ptr<const argparse::ArgumentParser> parse_arguments(int argc, char *argv[])
{
    auto parser = std::make_shared<argparse::ArgumentParser>("mount.sfs");

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
    
    return 0;
}

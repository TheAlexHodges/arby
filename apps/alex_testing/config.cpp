//
// Created by ahodges on 26/05/22.
//

#include "config.hpp"

namespace app
{
config
config::load(const arby::fs::path &path)
{
    if (!is_regular_file(path))
        throw std::runtime_error("cannot find config file");

    // Get filesize
    auto size = arby::fs::file_size(path);
    if (size > MAX_CONFIG_FILE_SIZE)
        throw std::runtime_error("config file too large");

    // Read file
    auto stream = arby::fs::ifstream(path);
    if (!stream.good())
        throw std::runtime_error("unable to open file stream");
    auto buf = buffer();
    buf.resize(size);
    stream.read(buf.data(), size);
    stream.close();

    // Parse json
    auto j = arby::json::parse(arby::json::string_view(buf.data(), buf.size()));

    return config(std::move(j), std::move(buf));
}
};   // namespace app
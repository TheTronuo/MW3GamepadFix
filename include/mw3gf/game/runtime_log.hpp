#pragma once
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>
namespace mw3gf::game {
class RuntimeLog {
  public:
    explicit RuntimeLog(const std::filesystem::path& path);
    void write(std::string_view text) noexcept;

  private:
    std::ofstream stream_;
    std::mutex mutex_;
};
} // namespace mw3gf::game

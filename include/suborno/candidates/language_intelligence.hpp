#pragma once

#include <string>
#include <vector>

namespace suborno::dictionary {
class Dictionary;
}

namespace suborno::candidates {

class LanguageIntelligence {
  public:
    explicit LanguageIntelligence(const dictionary::Dictionary& dictionary);

    [[nodiscard]] bool is_known_word(const std::string& normalized_word) const;
    [[nodiscard]] std::vector<std::string> suggest_prefix(const std::string& prefix) const;
    [[nodiscard]] std::string correct_or_original(const std::string& normalized_word) const;

  private:
    const dictionary::Dictionary& dictionary_;
};

}  // namespace suborno::candidates

#include <replxx.hxx>
using namespace replxx;

int main() {
    Replxx rx;

    // Callback wywoływany natychmiast po naciśnięciu ':'
    auto colon_handler = [&](char32_t ch) -> Replxx::ACTION_RESULT {
        // tu twoja akcja, np. wejście w tryb komend
        return Replxx::ACTION_RESULT::CONTINUE; // lub BAIL żeby zakończyć wejście
    };

    rx.bind_key(':', colon_handler);
    // normalnie wczytuje linię z historią, edycją, aż do Enter
    const char* line = rx.input(">> ");
}

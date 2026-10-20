#include "./Editor.h"

int main(int, char**) {
    eokas::Editor editor;
    if (!editor.init())
        return 1;

    editor.quit();
    return 0;
}

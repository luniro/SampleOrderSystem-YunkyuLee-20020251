#pragma once

namespace Console {
    // Returns the current console height (number of lines).
    // Returns 24 if the query fails (e.g. non-terminal / redirected output).
    int get_height();
}

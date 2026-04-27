#pragma once

#include "oc/app.h"
#include "platform/drivers/weegfx.h"

namespace oc_test_pages {

class PageApp : public oc::Application {
public:
    explicit PageApp(const char* title) : title_(title) {}
    const char* title() const { return title_; }
    virtual void draw_body(weegfx::Graphics& gfx) = 0;

private:
    const char* title_;
};

} // namespace oc_test_pages

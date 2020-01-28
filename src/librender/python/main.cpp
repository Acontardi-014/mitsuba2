#include <mitsuba/python/python.h>

MTS_PY_DECLARE(Spiral);

PYBIND11_MODULE(render_ext, m) {
    // Temporarily change the module name (for pydoc)
    m.attr("__name__") = "mitsuba.render";

    MTS_PY_IMPORT(Spiral);

    // Change module name back to correct value
    m.attr("__name__") = "mitsuba.render_ext";
}

#include <mitsuba/core/vector.h>
#include <mitsuba/python/python.h>

MTS_PY_EXPORT_FLOAT_VARIANTS(vector) {
    MTS_IMPORT_CORE_TYPES()
    m.def("coordinate_system",
          vectorize<Float>(&coordinate_system<Vector3f>),
          "n"_a, D(coordinate_system));
}

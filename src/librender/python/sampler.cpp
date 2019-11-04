#include <mitsuba/python/python.h>
#include <mitsuba/render/sampler.h>

MTS_PY_EXPORT_VARIANTS(Sampler) {
    MTS_IMPORT_TYPES()
    MTS_IMPORT_OBJECT_TYPES()
    MTS_PY_CLASS(Sampler, Object)
        .def_method(Sampler, clone)
        .def_method(Sampler, seed, "seed"_a, "size"_a)
        .def_method(Sampler, next_1d, "active"_a)
        .def_method(Sampler, next_2d, "active"_a)
        .def_method(Sampler, sample_count)
        ;
}

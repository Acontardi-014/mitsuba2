import numpy as np


def check_vectorization(func, pdf_func, func_vec, pdf_func_vec, resolution = 2):
    """
    Helper routine which compares evaluations of the vectorized and
    non-vectorized version of a warping routine
    """
    # Generate resolution^2 test points on a 2D grid
    t = np.linspace(1e-3, 1, resolution)
    x, y = np.meshgrid(t, t)
    samples = np.float32(np.column_stack((x.ravel(), y.ravel())))

    # Run the sampling routine
    result = func_vec(samples)

    # Evaluate the PDF
    pdf = pdf_func_vec(result)

    # Check against the scalar version
    for i in range(samples.shape[1]):
        assert np.allclose(result[i, :], func(samples[i, :]), atol=1e-4)
        assert np.allclose(pdf[i], pdf_func(result[i, :]), atol=1e-6)


def check_inverse(func, inverse):
    for x in np.linspace(1e-6, 1-1e-6, 10):
        for y in np.linspace(1e-6, 1-1e-6, 10):
            p1 = np.array([x, y])
            p2 = func(p1)
            p3 = inverse(p2)
            assert(np.allclose(p1, p3, atol=1e-5))


def test_square_to_uniform_disk():
    from mitsuba.scalar_rgb.core.warp import square_to_uniform_disk, \
                                             uniform_disk_to_square, \
                                             square_to_uniform_disk_pdf

    assert(np.allclose(square_to_uniform_disk([0.5, 0]), [0, 0]))
    assert(np.allclose(square_to_uniform_disk([0, 1]),   [1, 0]))
    assert(np.allclose(square_to_uniform_disk([0.5, 1]), [-1, 0], atol=1e-7))
    assert(np.allclose(square_to_uniform_disk([1, 1]),   [1, 0], atol=1e-6))

    check_inverse(square_to_uniform_disk, uniform_disk_to_square)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_uniform_disk as square_to_uniform_disk_vec, \
                                                 square_to_uniform_disk_pdf as square_to_uniform_disk_pdf_vec
    except ImportError:
        pass

    check_vectorization(square_to_uniform_disk, square_to_uniform_disk_pdf, \
                        square_to_uniform_disk_vec, square_to_uniform_disk_pdf_vec)


def test_square_to_uniform_disk_concentric():
    from mitsuba.scalar_rgb.core.warp import square_to_uniform_disk_concentric, \
                                             square_to_uniform_disk_pdf, \
                                             uniform_disk_to_square_concentric
    from math import sqrt

    assert(np.allclose(square_to_uniform_disk_concentric([0, 0]), ([-1 / sqrt(2),  -1 / sqrt(2)])))
    assert(np.allclose(square_to_uniform_disk_concentric([0.5, .5]), [0, 0]))

    check_inverse(square_to_uniform_disk_concentric, uniform_disk_to_square_concentric)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_uniform_disk_concentric as square_to_uniform_disk_concentric_vec, \
                                                 square_to_uniform_disk_pdf as square_to_uniform_disk_pdf_vec
    except ImportError:
        pass

    check_vectorization(square_to_uniform_disk_concentric, square_to_uniform_disk_pdf, \
                        square_to_uniform_disk_concentric_vec, square_to_uniform_disk_pdf_vec)


def test_square_to_uniform_triangle():
    from mitsuba.scalar_rgb.core.warp import square_to_uniform_triangle, \
                                  square_to_uniform_triangle_pdf, \
                                  uniform_triangle_to_square
    assert(np.allclose(square_to_uniform_triangle([0, 0]),   [0, 0]))
    assert(np.allclose(square_to_uniform_triangle([0, 0.1]), [0, 0.1]))
    assert(np.allclose(square_to_uniform_triangle([0, 1]),   [0, 1]))
    assert(np.allclose(square_to_uniform_triangle([1, 0]),   [1, 0]))
    assert(np.allclose(square_to_uniform_triangle([1, 0.5]), [1, 0]))
    assert(np.allclose(square_to_uniform_triangle([1, 1]),   [1, 0]))

    check_inverse(square_to_uniform_triangle, uniform_triangle_to_square)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_uniform_triangle as square_to_uniform_triangle_vec, \
                                                 square_to_uniform_triangle_pdf as square_to_uniform_triangle_pdf_vec
    except ImportError:
        pass

    check_vectorization(square_to_uniform_triangle, square_to_uniform_triangle_pdf, \
                        square_to_uniform_triangle_vec, square_to_uniform_triangle_pdf_vec)


def test_interval_to_tent():
    from mitsuba.scalar_rgb.core.warp import interval_to_tent
    assert(np.allclose(interval_to_tent(0.5), 0))
    assert(np.allclose(interval_to_tent(0),   -1))
    assert(np.allclose(interval_to_tent(1),   1))


def test_interval_to_nonuniform_tent():
    from mitsuba.scalar_rgb.core.warp import interval_to_nonuniform_tent
    assert(np.allclose(interval_to_nonuniform_tent(0, 0.5, 1, 0.499), 0.499, atol=1e-3))
    assert(np.allclose(interval_to_nonuniform_tent(0, 0.5, 1, 0), 0))
    assert(np.allclose(interval_to_nonuniform_tent(0, 0.5, 1, 0.5), 1))


def test_square_to_tent():
    from mitsuba.scalar_rgb.core.warp import square_to_tent, \
                                  square_to_tent_pdf, \
                                  tent_to_square
    assert(np.allclose(square_to_tent([0.5, 0.5]), [0, 0]))
    assert(np.allclose(square_to_tent([0, 0.5]), [-1, 0]))
    assert(np.allclose(square_to_tent([1, 0]), [1, -1]))

    check_inverse(square_to_tent, tent_to_square)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_tent as square_to_tent_vec, \
                                                 square_to_tent_pdf as square_to_tent_pdf_vec
    except ImportError:
        pass

    check_vectorization(square_to_tent, square_to_tent_pdf, \
                        square_to_tent_vec, square_to_tent_pdf_vec)


def test_square_to_uniform_sphere_vec():
    from mitsuba.scalar_rgb.core.warp import square_to_uniform_sphere, \
                                  square_to_uniform_sphere_pdf, \
                                  uniform_sphere_to_square

    assert(np.allclose(square_to_uniform_sphere([0, 0]), [0, 0,  1]))
    assert(np.allclose(square_to_uniform_sphere([0, 1]), [0, 0, -1]))
    assert(np.allclose(square_to_uniform_sphere([0.5, 0.5]), [-1, 0, 0], atol=1e-7))

    check_inverse(square_to_uniform_sphere, uniform_sphere_to_square)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_uniform_sphere as square_to_uniform_sphere_vec, \
                                                 square_to_uniform_sphere_pdf as square_to_uniform_sphere_pdf_vec
    except ImportError:
        pass

    check_vectorization(square_to_uniform_sphere, square_to_uniform_sphere_pdf, \
                        square_to_uniform_sphere_vec, square_to_uniform_sphere_pdf_vec)


def test_square_to_uniform_hemisphere():
    from mitsuba.scalar_rgb.core.warp import square_to_uniform_hemisphere, \
                                  square_to_uniform_hemisphere_pdf, \
                                  uniform_hemisphere_to_square

    assert(np.allclose(square_to_uniform_hemisphere([0.5, 0.5]), [0, 0, 1]))
    assert(np.allclose(square_to_uniform_hemisphere([0, 0.5]), [-1, 0, 0]))

    check_inverse(square_to_uniform_hemisphere, uniform_hemisphere_to_square)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_uniform_hemisphere as square_to_uniform_hemisphere_vec, \
                                                 square_to_uniform_hemisphere_pdf as square_to_uniform_hemisphere_pdf_vec
    except ImportError:
        pass

    check_vectorization(square_to_uniform_hemisphere, square_to_uniform_hemisphere_pdf, \
                        square_to_uniform_hemisphere_vec, square_to_uniform_hemisphere_pdf_vec)


def test_square_to_cosine_hemisphere():
    from mitsuba.scalar_rgb.core.warp import square_to_cosine_hemisphere, \
                                  square_to_cosine_hemisphere_pdf, \
                                  cosine_hemisphere_to_square

    assert(np.allclose(square_to_cosine_hemisphere([0.5, 0.5]), [0,  0,  1]))
    assert(np.allclose(square_to_cosine_hemisphere([0.5,   0]), [0, -1, 0], atol=1e-7))

    check_inverse(square_to_cosine_hemisphere, cosine_hemisphere_to_square)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_cosine_hemisphere as square_to_cosine_hemisphere_vec, \
                                                 square_to_cosine_hemisphere_pdf as square_to_cosine_hemisphere_pdf_vec
    except ImportError:
        pass

    check_vectorization(square_to_cosine_hemisphere, square_to_cosine_hemisphere_pdf, \
                        square_to_cosine_hemisphere_vec, square_to_cosine_hemisphere_pdf_vec)


def test_square_to_uniform_cone():
    from mitsuba.scalar_rgb.core.warp import square_to_uniform_cone, \
                                  square_to_uniform_cone_pdf, \
                                  uniform_cone_to_square

    assert(np.allclose(square_to_uniform_cone([0.5, 0.5], 1), [0, 0, 1]))
    assert(np.allclose(square_to_uniform_cone([0.5, 0],   1), [0, 0, 1], atol=1e-7))
    assert(np.allclose(square_to_uniform_cone([0.5, 0],   0), [0, -1, 0], atol=1e-7))

    fwd = lambda v: square_to_uniform_cone(v, 0.3)
    pdf = lambda v: square_to_uniform_cone_pdf(v, 0.3)
    inv = lambda v: uniform_cone_to_square(v, 0.3)

    check_inverse(fwd, inv)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_uniform_cone as square_to_uniform_cone_vec, \
                                                 square_to_uniform_cone_pdf as square_to_uniform_cone_pdf_vec
    except ImportError:
        pass

    fwd_vec = lambda v: square_to_uniform_cone_vec(v, 0.3)
    pdf_vec = lambda v: square_to_uniform_cone_pdf_vec(v, 0.3)

    check_vectorization(fwd, pdf, fwd_vec, pdf_vec)


def test_square_to_beckmann():
    from mitsuba.scalar_rgb.core.warp import square_to_beckmann, \
                                  square_to_beckmann_pdf, \
                                  beckmann_to_square

    fwd = lambda v: square_to_beckmann(v, 0.3)
    pdf = lambda v: square_to_beckmann_pdf(v, 0.3)
    inv = lambda v: beckmann_to_square(v, 0.3)

    check_inverse(fwd, inv)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_beckmann as square_to_beckmann_vec, \
                                                 square_to_beckmann_pdf as square_to_beckmann_pdf_vec
    except ImportError:
        pass

    fwd_vec = lambda v: square_to_beckmann_vec(v, 0.3)
    pdf_vec = lambda v: square_to_beckmann_pdf_vec(v, 0.3)

    check_vectorization(fwd, pdf, fwd_vec, pdf_vec)


def test_square_to_von_mises_fisher():
    from mitsuba.scalar_rgb.core.warp import square_to_von_mises_fisher, \
                                  square_to_von_mises_fisher_pdf, \
                                  von_mises_fisher_to_square

    fwd = lambda v: square_to_von_mises_fisher(v, 10)
    pdf = lambda v: square_to_von_mises_fisher_pdf(v, 10)
    inv = lambda v: von_mises_fisher_to_square(v, 10)

    check_inverse(fwd, inv)

    try:
        from mitsuba.packet_rgb.core.warp import square_to_von_mises_fisher as square_to_von_mises_fisher_vec, \
                                                 square_to_von_mises_fisher_pdf as square_to_von_mises_fisher_pdf_vec
    except ImportError:
        pass

    fwd_vec = lambda v: square_to_von_mises_fisher_vec(v, 10)
    pdf_vec = lambda v: square_to_von_mises_fisher_pdf_vec(v, 10)

    check_vectorization(fwd, pdf, fwd_vec, pdf_vec)


def test_square_to_std_normal_pdf():
    from mitsuba.scalar_rgb.core.warp import square_to_std_normal_pdf
    assert(np.allclose(square_to_std_normal_pdf([0, 0]),   0.16, atol=1e-2))
    assert(np.allclose(square_to_std_normal_pdf([0, 0.8]), 0.12, atol=1e-2))
    assert(np.allclose(square_to_std_normal_pdf([0.8, 0]), 0.12, atol=1e-2))


def test_square_to_std_normal():
    from mitsuba.scalar_rgb.core.warp import square_to_std_normal
    assert(np.allclose(square_to_std_normal([0, 0]), [0, 0]))
    assert(np.allclose(square_to_std_normal([0, 1]), [0, 0]))
    assert(np.allclose(square_to_std_normal([0.39346, 0]), [1, 0], atol=1e-3))


def test_hierarchical_warp():
    try:
        from mitsuba.packet_rgb.core.warp import Hierarchical2D0
    except ImportError:
        pytest.skip("packet_rgb mode not enabled")

    np.random.seed(0)
    data = np.random.rand(7, 3)
    distr = Hierarchical2D0(data)
    sample_in = np.random.rand(10, 2)
    sample_out, pdf_out = distr.sample(sample_in)
    assert np.allclose(pdf_out, distr.eval(sample_out))
    assert np.allclose(sample_in, distr.invert(sample_out)[0])


def test_marginal_warp():
    try:
        from mitsuba.packet_rgb.core.warp import Marginal2D0
    except ImportError:
        pytest.skip("packet_rgb mode not enabled")

    np.random.seed(0)
    data = np.random.rand(7, 3)
    distr = Marginal2D0(data)
    sample_in = np.random.rand(10, 2)
    sample_out, pdf_out = distr.sample(sample_in)
    print(pdf_out)
    print(distr.eval(sample_out))
    assert np.allclose(pdf_out, distr.eval(sample_out))
    assert np.allclose(sample_in, distr.invert(sample_out)[0])

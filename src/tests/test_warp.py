import numpy as np

def check_vectorization(func, pdf_func, resolution = 10):
    """
    Helper routine which compares evaluations of the vectorized and
    non-vectorized version of a warping routine
    """
    # Generate resolution^2 test points on a 2D grid
    t = np.linspace(0, 1, resolution)
    x, y = np.meshgrid(t, t)
    samples = np.float32(np.column_stack((x.ravel(), y.ravel())))

    # Run the sampling routine
    result = func(samples)

    # Evaluate the PDF
    pdf = pdf_func(result)

    # Check against the scalar version
    for i in range(samples.shape[1]):
        assert np.allclose(result[i, :], func(samples[i, :]))
        pdf_func(result[i, :])
        assert np.allclose(pdf[i], pdf_func(result[i, :]))

def test_square_to_uniform_sphere():
    from mitsuba.core.warp import square_to_uniform_sphere

    assert(np.allclose(square_to_uniform_sphere([0, 0]), [0, 0,  1]))
    assert(np.allclose(square_to_uniform_sphere([0, 1]), [0, 0, -1]))
    assert(np.allclose(square_to_uniform_sphere([0.5, 0.5]), [-1, 0, 0], atol=1e-7))

def test_square_to_uniform_sphere_vec():
    from mitsuba.core.warp import square_to_uniform_sphere
    from mitsuba.core.warp import square_to_uniform_sphere_pdf
    check_vectorization(square_to_uniform_sphere, square_to_uniform_sphere_pdf)



def test_square_to_uniform_hemisphere():
    from mitsuba.core import square_to_uniform_hemisphere

    assert(np.allclose(square_to_uniform_hemisphere([0, 0]), [1, 0,  0]))
    assert(np.allclose(square_to_uniform_hemisphere([0, 1]), [0, 0, 1]))

def test_square_to_uniform_hemisphere_vec():
    from mitsuba.core import square_to_uniform_hemisphere
    from mitsuba.core import square_to_uniform_hemisphere_pdf
    check_vectorization(square_to_uniform_hemisphere, square_to_uniform_hemisphere_pdf)



def test_square_to_uniform_disk_concentric():
    from mitsuba.core import square_to_uniform_disk_concentric
    from math import sqrt

    assert(np.allclose(square_to_uniform_disk_concentric([0, 0]), ([-1 / sqrt(2),  -1 / sqrt(2)] )))
    assert(np.allclose(square_to_uniform_disk_concentric([0.5, .5]), [0, 0]))



def test_square_to_cosine_hemisphere():
    from mitsuba.core import square_to_cosine_hemisphere

    assert(np.allclose(square_to_cosine_hemisphere([0.5, 0.5]), [0,  0,  1]))
    assert(np.allclose(square_to_cosine_hemisphere([0.5,   0]), [0, -1, 0], atol=1e-7))
    

def test_square_to_uniform_cone():
    from mitsuba.core import square_to_uniform_cone

    assert(np.allclose(square_to_uniform_cone([0.5, 0.5], 1), [0, 0, 1]))
    assert(np.allclose(square_to_uniform_cone([0.5, 0],   1), [0, 0, 1], atol=1e-7))
    assert(np.allclose(square_to_uniform_cone([0.5, 0],   0), [0, 0, 1], atol=1e-7))
import os
import numpy as np
import mitsuba

# Set the desired mitsuba variant
mitsuba.set_variant('scalar_rgb')

from mitsuba.core  import Bitmap, Struct, Thread
from mitsuba.core .xml import load_file


SCENE_DIR = '../../../resources/data/scenes/'
filename = os.path.join(SCENE_DIR, 'cbox/cbox.xml')

# Append the directory containing the scene to the "file resolver".
# This is needed since the scene specifies meshes and textures
# using relative paths.
directory_name = os.path.dirname(filename)
Thread.thread().file_resolver().append(directory_name)

# Load the actual scene
scene = load_file(filename)

# Call the scene's integrator to render the loaded scene
scene.integrator().render(scene, scene.sensors()[0])

# After rendering, the rendered data is stored in the film
film = scene.sensors()[0].film()

# Write out rendering as high dynamic range OpenEXR file
film.set_destination_file('cbox.exr')
film.develop()

# Write out a tonemapped JPG of the same rendering
bmp = film.bitmap()
bmp.convert(Bitmap.PixelFormat.RGB, Struct.Type.UInt8, srgb_gamma=True).write('cbox.jpg')

# Get linear pixel values as a numpy array for further processing
bmp_linear_rgb = bmp.convert(Bitmap.PixelFormat.RGB, Struct.Type.Float32, srgb_gamma=False)
image_np = np.array(bmp_linear_rgb)
print(image_np.shape)
